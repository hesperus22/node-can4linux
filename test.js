var Can = require('./can.js');

var can = new Can({device: process.argv[2], self:true});
can.on('data', function(data){
    console.log(data);
});
can.on('error', function(err){
    console.log(err);
});

