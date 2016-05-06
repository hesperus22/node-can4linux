var Can = require('./can.js');

var can = new Can({self:true});
can.on('data', function(data){
    console.log(data);
});
can.on('error', function(err){
    console.log(err);
});

can.send({id: 16285706, ext:1});

