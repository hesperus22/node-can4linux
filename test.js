var Can = require('./can.js');

var can = new Can({device: 'c:/Users/probakowski/Documents/private/test.txt'});
can.on('data', function(data){
    console.log(data);
});
can.on('error', function(err){
    console.log(err);
});
can.send({ id: 5, data: [1,2,3,4]});
setTimeout(function(){
    can.close();
}, 3000);