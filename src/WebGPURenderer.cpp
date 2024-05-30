#include "WebGPURenderer.h"
#include "Common.h"


#ifdef __EMSCRIPTEN__
    #include <emscripten.h>
    #include <emscripten/html5.h>
    #include <emscripten/html5_webgpu.h>
    #include <webgpu/webgpu_cpp.h>
#endif

#undef NDEBUG

#include <cassert>
#include <cstdio>
#include <cstdlib>
#include <array>
#include <memory>
#include <iostream>


static const std::array<float, 18> g_VertexAttrsData = {
//     x,     y,   z     r,   g,   b
    -0.5f, -0.5f, 0.f, 1.f, 0.f, 0.f,
     0.5f, -0.5f, 0.f, 0.f, 1.f, 0.f,
     0.0f,  0.5f, 0.f, 0.f, 0.f, 1.f,
}; 

struct MyUniforms {
	float time;
};

static const char shaderCode[] = R"(
    struct MyUniforms {
        time: f32
    };

    @group(0) @binding(0) var<uniform> u_MyUniforms: MyUniforms;

    struct VertexInput {
        @location(0) position: vec3f,
        @location(1) color: vec3f,
    };

    struct VertexOutput {
        @builtin(position) position: vec4f,
        @location(0) color: vec3f,
    };

    @vertex
    fn vs_main(in: VertexInput) -> VertexOutput {
        let angle: f32 = radians(0.2) * u_MyUniforms.time;
        let rotationMatrix = mat4x4<f32>(
            cos(angle), -sin(angle), 0.0, 0.0,
            sin(angle),  cos(angle), 0.0, 0.0,
            0.0,        0.0,         1.0, 0.0,
            0.0,        0.0,         0.0, 1.0
        );

        var out: VertexOutput;
        out.position = rotationMatrix * vec4<f32>(in.position, 1.0);
        out.color = in.color;
        return out;
    }

    @fragment
    fn fs_main(in: VertexOutput) -> @location(0) vec4f {
        return vec4<f32>(in.color, 1.0);
    }
)";

// static const wgpu::Instance g_Instance = wgpuCreateInstance(nullptr);
static wgpu::Instance g_Instance;
static wgpu::Device g_Device;
static wgpu::Queue g_Queue;
static wgpu::SwapChain g_SwapChain;  // 渲染帧时，当需要修改画布尺寸，则需要修改，因此不能是const类型的变量
static wgpu::TextureView g_CanvasDepthStencilView;   // 渲染时，需要每帧获取修改，因此不能是const类型的变量
static wgpu::RenderPipeline g_Pipeline;

static int testsCompleted = 0;
static wgpu::Buffer readbackBuffer;

static wgpu::Buffer g_VertexBuffer;
static wgpu::BindGroup g_BindGroup;
static MyUniforms g_Uniforms;
static wgpu::Buffer g_UniformBuffer;


void GetDevice(AdapterAndDeviceCallback callback)
{
    auto requestAdapterCallback = [](WGPURequestAdapterStatus status, WGPUAdapter cAdapter, const char *message, void *userdata)
    {
        if (status != WGPURequestAdapterStatus_Success) exit(0);

        auto requestDeviceCallback = [](WGPURequestDeviceStatus status, WGPUDevice cDevice, const char *message, void *userdata)
        {
            wgpu::Device device = wgpu::Device::Acquire(cDevice);
            device.SetUncapturedErrorCallback(
                [](WGPUErrorType type, const char *message, void *userdata)
                {
                    std::cout << "Error: " << type << " , message: " << message;
                },
                nullptr
            );
            AdapterAndDeviceCallback callback = *reinterpret_cast<AdapterAndDeviceCallback*>(userdata);
            callback(device);
        };

        //
        wgpu::Adapter adapter = wgpu::Adapter::Acquire(cAdapter);
        adapter.RequestDevice(nullptr, requestDeviceCallback, userdata);
    };

    g_Instance.RequestAdapter(nullptr, requestAdapterCallback, new AdapterAndDeviceCallback(callback));
}

void init() {
    g_Device.SetUncapturedErrorCallback(
        [](WGPUErrorType errorType, const char* message, void*) {
            printf("%d: %s\n", errorType, message);
        }, nullptr);

    g_Queue = g_Device.GetQueue();

    wgpu::ShaderModule shaderModule{};
    {
        wgpu::ShaderModuleWGSLDescriptor wgslDesc{};
        wgslDesc.code = shaderCode;

        wgpu::ShaderModuleDescriptor descriptor{};
        descriptor.nextInChain = &wgslDesc;
        shaderModule = g_Device.CreateShaderModule(&descriptor);
        shaderModule.GetCompilationInfo([](WGPUCompilationInfoRequestStatus status, const WGPUCompilationInfo* info, void*) {
            assert(status == WGPUCompilationInfoRequestStatus_Success);
            assert(info->messageCount == 0);
            std::printf("Shader compile succeeded\n");
        }, nullptr);
    }

    wgpu::BindGroupLayout bindGroupLayout;
    {
        // Create binding layout (don't forget to = Default)
        wgpu::BindGroupLayoutEntry bindGroupLayoutEntry = {
            .binding = 0,
            .visibility = wgpu::ShaderStage::Vertex | wgpu::ShaderStage::Fragment,
            .buffer = {
                .type = wgpu::BufferBindingType::Uniform,
                .minBindingSize = sizeof(MyUniforms)
            }
        };

        wgpu::BindGroupLayoutDescriptor bindGroupLayoutDesc = {
            .entryCount = 1,
            .entries = &bindGroupLayoutEntry
        };
        bindGroupLayout = g_Device.CreateBindGroupLayout(&bindGroupLayoutDesc);

         // Create uniform buffer
        wgpu::BufferDescriptor bufferDesc = {
            .usage = wgpu::BufferUsage::CopyDst | wgpu::BufferUsage::Uniform,
            .size = sizeof(MyUniforms),
            .mappedAtCreation = false
        };
        g_UniformBuffer = g_Device.CreateBuffer(&bufferDesc);

        // Upload the initial value of the uniforms
        g_Uniforms.time = 1.0f;
        g_Queue.WriteBuffer(g_UniformBuffer, 0, &g_Uniforms, sizeof(MyUniforms));

        // Create a binding
        wgpu::BindGroupEntry bindingGroupEntry = {
            .binding = 0,
            .buffer = g_UniformBuffer,
            .offset = 0,
            .size = sizeof(MyUniforms)
        };

        // A bind group contains one or multiple bindings
        wgpu::BindGroupDescriptor bindGroupDesc = {
            .layout = bindGroupLayout,
            .entryCount = bindGroupLayoutDesc.entryCount,
            .entries = &bindingGroupEntry
        };
        g_BindGroup = g_Device.CreateBindGroup(&bindGroupDesc);
    }

    wgpu::VertexBufferLayout vertexBufferLayout;
    {
        wgpu::PipelineLayoutDescriptor pl{};
        pl.bindGroupLayoutCount = 1;
        // pl.bindGroupLayouts = nullptr;
        pl.bindGroupLayouts = (wgpu::BindGroupLayout*)&bindGroupLayout;

        // Vertex fetch
        std::vector<wgpu::VertexAttribute> vertexAttrs(2);

        // Position attribute
        vertexAttrs[0].shaderLocation = 0;
        vertexAttrs[0].format = wgpu::VertexFormat::Float32x3;
        vertexAttrs[0].offset = 0;

        // Color attribute
        vertexAttrs[1].shaderLocation = 1;
        vertexAttrs[1].format = wgpu::VertexFormat::Float32x3;
        vertexAttrs[1].offset = 3 * sizeof(float);

        // layout
        vertexBufferLayout.attributeCount = (uint32_t)vertexAttrs.size();
        vertexBufferLayout.attributes = vertexAttrs.data();
        vertexBufferLayout.arrayStride = 6 * sizeof(float);
        vertexBufferLayout.stepMode = wgpu::VertexStepMode::Vertex;

        // Create vertex buffer
        wgpu::BufferDescriptor bufferDesc;
        bufferDesc.size = g_VertexAttrsData.size() * sizeof(float);
        bufferDesc.usage = wgpu::BufferUsage::CopyDst | wgpu::BufferUsage::Vertex;
        bufferDesc.mappedAtCreation = false;
        g_VertexBuffer = g_Device.CreateBuffer(&bufferDesc);
        g_Queue.WriteBuffer(g_VertexBuffer, 0, g_VertexAttrsData.data(), bufferDesc.size);

        //
        wgpu::ColorTargetState colorTargetState{};
        colorTargetState.format = wgpu::TextureFormat::BGRA8Unorm;

        wgpu::DepthStencilState depthStencilState{};
        depthStencilState.format = wgpu::TextureFormat::Depth32Float;
        depthStencilState.depthCompare = wgpu::CompareFunction::Always;

        wgpu::FragmentState fragmentState{};
        fragmentState.module = shaderModule;
        fragmentState.entryPoint = "fs_main";
        fragmentState.targetCount = 1;
        fragmentState.targets = &colorTargetState;


        //
        wgpu::RenderPipelineDescriptor pipelineDesc{};
        pipelineDesc.layout = g_Device.CreatePipelineLayout(&pl);

        pipelineDesc.vertex.bufferCount = 1;
        pipelineDesc.vertex.buffers = &vertexBufferLayout;
        pipelineDesc.vertex.module = shaderModule;
        pipelineDesc.vertex.entryPoint = "vs_main";
        pipelineDesc.vertex.constantCount = 0;
        pipelineDesc.vertex.constants = nullptr;

        pipelineDesc.fragment = &fragmentState;

        pipelineDesc.depthStencil = &depthStencilState;

        //
        pipelineDesc.primitive.topology = wgpu::PrimitiveTopology::TriangleList;
        pipelineDesc.primitive.stripIndexFormat = wgpu::IndexFormat::Undefined;
        pipelineDesc.primitive.frontFace = wgpu::FrontFace::CCW;
        pipelineDesc.primitive.cullMode = wgpu::CullMode::None;

        //
        g_Pipeline = g_Device.CreateRenderPipeline(&pipelineDesc);
    }
}

// The depth stencil attachment isn't really needed to draw the triangle
// and doesn't really affect the render result.
// But having one should give us a slightly better test coverage for the compile of the depth stencil descriptor.
void render(wgpu::TextureView view, wgpu::TextureView depthStencilView) {
    float currentTime = static_cast<float>(EM_ASM_DOUBLE(
        return performance.now();
    ));
    g_Uniforms.time = currentTime;
    g_Queue.WriteBuffer(g_UniformBuffer, 0, &g_Uniforms, sizeof(MyUniforms));

    wgpu::RenderPassColorAttachment attachment{};
    attachment.view = view;
    attachment.loadOp = wgpu::LoadOp::Clear;
    attachment.storeOp = wgpu::StoreOp::Store;
    attachment.clearValue = {0, 0, 0, 1};

    wgpu::RenderPassDescriptor renderpass{};
    renderpass.colorAttachmentCount = 1;
    renderpass.colorAttachments = &attachment;

    wgpu::RenderPassDepthStencilAttachment depthStencilAttachment = {};
    depthStencilAttachment.view = depthStencilView;
    depthStencilAttachment.depthClearValue = 0;
    depthStencilAttachment.depthLoadOp = wgpu::LoadOp::Clear;
    depthStencilAttachment.depthStoreOp = wgpu::StoreOp::Store;

    renderpass.depthStencilAttachment = &depthStencilAttachment;

    wgpu::CommandBuffer commands;
    {
        wgpu::CommandEncoder encoder = g_Device.CreateCommandEncoder();
        {
            wgpu::RenderPassEncoder pass = encoder.BeginRenderPass(&renderpass);
            pass.SetPipeline(g_Pipeline);
            pass.SetVertexBuffer(0, g_VertexBuffer, 0, g_VertexAttrsData.size() * sizeof(float));
            pass.SetBindGroup(0, g_BindGroup, 0, nullptr);
            pass.Draw(3);
            pass.End();
        }
        commands = encoder.Finish();
    }

    g_Queue.Submit(1, &commands);
}

void frame() {
    wgpu::TextureView backbuffer = g_SwapChain.GetCurrentTextureView();
    render(backbuffer, g_CanvasDepthStencilView);

    // TODO: Read back from the canvas with drawImage() (or something) and
    // check the result.

    // emscripten_cancel_main_loop();

    // exit(0) (rather than emscripten_force_exit(0)) ensures there is no dangling keepalive.
    // exit(0);
}

void run(const std::string& canvasId, uint32_t width, uint32_t height) {
    init();

    {
        wgpu::SurfaceDescriptorFromCanvasHTMLSelector canvasDesc{};
        // canvasDesc.selector = "#canvas";
        // std::cout << "323---》" << g_pHwd << std::endl;
        // canvasDesc.selector = g_pHwd.c_str();
        canvasDesc.selector = canvasId.c_str();

        wgpu::SurfaceDescriptor surfDesc{};
        surfDesc.nextInChain = &canvasDesc;
        wgpu::Surface surface = g_Instance.CreateSurface(&surfDesc);

        wgpu::SwapChainDescriptor scDesc{};
        scDesc.usage = wgpu::TextureUsage::RenderAttachment;
        scDesc.format = wgpu::TextureFormat::BGRA8Unorm;
        // scDesc.width = g_Width;
        // scDesc.height = g_Height;
        scDesc.width = width;
        scDesc.height = height;
        scDesc.presentMode = wgpu::PresentMode::Fifo;
        g_SwapChain = g_Device.CreateSwapChain(surface, &scDesc);

        {
            wgpu::TextureDescriptor descriptor{};
            descriptor.usage = wgpu::TextureUsage::RenderAttachment;
            // descriptor.size = { g_Width, g_Height, 1 };
            descriptor.size = { width, height, 1 };
            descriptor.format = wgpu::TextureFormat::Depth32Float;
            g_CanvasDepthStencilView = g_Device.CreateTexture(&descriptor).CreateView();
        }
    }
    emscripten_set_main_loop(frame, 0, false);
}

void* initWebGPURenderer(void *args)
{
    g_Instance = wgpu::CreateInstance(nullptr);

    //
    auto initArgs = static_cast<InitArgs*>(args);
    // initArgs不能按引用传递，否则，无法获取到值
    GetDevice([initArgs](wgpu::Device dev) {
        g_Device = dev;
        run(initArgs->canvasId, initArgs->width, initArgs->height);
    });

    return nullptr;
}

// #Ref:
// webgpu basic rendering example: https://github.com/emscripten-core/emscripten/blob/main/test/webgpu_basic_rendering.cpp
// A simple wgpu example：https://github.com/eliemichel/LearnWebGPU-Code/tree/step050
// webgpu cross platform demo：https://github.com/kainino0x/webgpu-cross-platform-demo