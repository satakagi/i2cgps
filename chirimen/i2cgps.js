// @ts-check
// I2CGPS driver for CHIRIMEN
// GPS reciever via I2C 
// based on 
// Programmed by Satoru Takagi

/** @param {number} ms Delay for a number of milliseconds. */
const sleep = ms => new Promise(resolve => setTimeout(resolve, ms));

var I2CGPS = function (i2cPort, slaveAddress) {
    if (!slaveAddress) {
        slaveAddress = 0x58;
    }
    this.i2cPort = i2cPort;
    this.i2cSlave = null;
    this.slaveAddress = slaveAddress;
}

I2CGPS.prototype = {
    init: async function () {
        this.i2cSlave = await this.i2cPort.open(this.slaveAddress);
    },
    getInt16(b0, b1) {
        return (b0 + (b1 << 8));
    },
    getInt32(b0, b1, b2, b3) {
        return (b0 + (b1 << 8) + (b2 << 16) + (b3 << 24));
    },
    readData: async function () {
        await this.i2cSlave.writeBytes([0xAC, 0x33, 0x00]); // read
        while (await this.i2cSlave.readByte() & 0x80) {
            await sleep(10);
        }
        var mdata = await this.i2cSlave.readBytes(22); // raw data

        var len = mdata[0]; // should br 25
        var stat = mdata[1];
        var year = this.getInt16(mdata[2], mdata[3]);
        var month = mdata[4];
        var day = mdata[5];
        var hour = mdata[6];
        var minute = mdata[7];
        var second = mdata[8];
        var latitude = this.getInt32(mdata[9], mdata[10], mdata[11], mdata[12]) / 10000000.0;
        var longitude = this.getInt32(mdata[13], mdata[14], mdata[15], mdata[16]) / 10000000.0;
        var altitude = this.getInt32(mdata[17], mdata[18], mdata[19], mdata[20]) / 1000.0; // m
        var speed = this.getInt32(mdata[21], mdata[22], mdata[23], mdata[24]) * (1.852 / 1000.0); // Km/hr

        //        console.log(mdata[0],mdata[1],mdata[2],mdata[3],mdata[3]<<8);
        if ( len == 25 ){
            return { stat, date: { year, month, day, hour, minute, second }, latitude, longitude, altitude, speed };
        //        return {raw:mdata,len,stat,date:{year,month,day,hour,minute,second},lat,lon,alt,speed};
        } else {
            return {stat:-1}
        }

        /** 
        return {
            latitude: lat,
            longitude: lng,
            level: lvl
        }
        **/
    }
};

export default I2CGPS;
