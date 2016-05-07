# can4linux for Node.js
[![Dependency Status](https://david-dm.org/voodootikigod/node-serialport.svg)](https://david-dm.org/voodootikigod/node-serialport)

***
This is simple wrapper for [can4linux library](https://sourceforge.net/projects/can4linux/). It provides methods only for reading and sending CAN messages. There is no option for setting baud rate etc. For support you can open a [github issue](https://github.com/hesperus22/node-can4linux/issues/new)

## Installation Instructions

You have to have everything in place for compiling native modules for node. Of course you also have to compile and start can4linux kernel module.
Then installation is easy:

```
npm install can4linux
```

## Usage

```
var Can = require('can4linux');

var can = new Can({
  device: '/dev/can0', //CAN device to use, can be omitted
  self: false, // turn to true to see message send by this device (with false only incomming messages are shown), can be omitted
});

can.on('data', function(data){
  console.log(data.id);   // CAN id
  console.log(data.ext);  // Is this extended id?
  console.log(data.rtr);  // Is this RTR message
  console.log(data.data); // Array with CAN data
  console.log(data.self); // Is this message send by this device?
  console.log(data.timestamp.sec + "s " + data.timestamp.usec + "us") // Timestamp
});

can.on('error', function(err){
  console.log("Error received", err);
  can.close();
});

can.send({
  id: 5,          // CAN id
  ext: false,     // Is this extended id?
  rtr: false,     // Is this RTR message
  data: [1,2,3]   // Max 8 bytes unless you have CAN FD enabled, can be omitted
});
