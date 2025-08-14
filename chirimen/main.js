import { requestI2CAccess } from "./node_modules/node-web-i2c/index.js";
import I2CGPS from "./i2cgps.js";
const sleep = (msec) => new Promise((resolve) => setTimeout(resolve, msec));

main();

async function main() {
	const i2cAccess = await requestI2CAccess();
	const port = i2cAccess.ports.get(1);
	const i2cgps = new I2CGPS(port, 0x58);
	await i2cgps.init();

	while (true) {
		var data = await i2cgps.readData();
		console.log(data);
		await sleep(1000);
	}
}
