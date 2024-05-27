import express from 'express';
import chalk from 'chalk';
import { program } from 'commander';

import path from "path";
import os from 'os';
import fs from 'fs';
import http from 'http';
import https from 'https';

function getPageFile(filename) {
  return filename.length > 0 ? filename : './index.html';
}

function getPort(port) {
  return port.length > 0 && !isNaN(port) ? port : 0;
}

function getMode(mode) {
  return (mode !== 'dev' && mode !== 'prod') ? 'prod' : mode;
}
 
function getIpv4Address() {
	const networkInterfaces = os.networkInterfaces();
	for (const item of networkInterfaces.WLAN) {
		if (item.family === 'IPv4') {
			return item.address;
		}
	}
}

// ES6 Module模式下，直接使用“__dirname”，或报错：ReferenceError: __dirname is not defined in ES module scope
const __dirname = path.resolve();

// 解析获取命令行输入的参数
program
  .name('server-util')
  .description('CLI to run serve')
  .version('0.1.0')
  .option('-f, --file <string>', 'Specify the HTML file to run.', getPageFile)
  .option('-p, --port <number>', 'Set http service port', getPort)
  .option('-s, --sslport <number>', 'Set https service port', getPort)
  .option('-m, --mode <string>', 'Get development mode.', getMode)

program.parse();
const options = program.opts();
const htmlPage = options.file;
const port = options.port;
const sslport = options.sslport;

// 创建express应用
const app = express();

// HTTP Server
const httpServer = http.createServer(app);

// HTTPS Server, 传入服务器私钥和服务器证书
const httpsServer = https.createServer({
	key: fs.readFileSync(path.join(__dirname, 'certs/test_server.key'), 'utf8'),
    cert: fs.readFileSync(path.join(__dirname, 'certs/test_server.cert'), 'utf8')
}, app);

// 配置Express
app.use(express.static(__dirname, {
    setHeaders: (res) => {
        // 设置响应头，以启用SharedArrayBuffer，从而使用多线程
        res.set('Cross-Origin-Opener-Policy', 'same-origin');
        res.set('Cross-Origin-Embedder-Policy', 'require-corp');
    }
}));

// 获取html页面
app.get('/', (req, res) => {
    // express跨域设置
    res.setHeader("Access-Control-Allow-Origin", "*");
    res.setHeader("Access-Control-Allow-Headers", "Origin, X-Requested-With, Content-Type, Accept");
    res.setHeader("Access-Control-Allow-Methods", "PUT,POST,GET,DELETE,OPTIONS");
    res.setHeader('Access-Control-Allow-Credentials', true); // 是否允许发送Cookie，默认false

    // 设置响应头，以启用SharedArrayBuffer，从而使用多线程
    res.setHeader("Cross-Origin-Opener-Policy", "same-origin");
    res.setHeader("Cross-Origin-Embedder-Policy", "require-corp");

    //
    res.sendFile(path.join(__dirname, htmlPage));    // 注意：路径不能以点开头!!!
});

// 监听端口
httpServer.listen(port, '0.0.0.0', () => {
    console.log('HTTP Server is running at:\n', 
      chalk.blue(`http://localhost:${port}\n`),
      chalk.blue(`http://127.0.0.1:${port}\n`));
});

httpsServer.listen(sslport, '0.0.0.0', () => {
    console.log('HTTPS Server is running at:\n', 
      chalk.blue(`https://${getIpv4Address()}:${sslport}`));
});
