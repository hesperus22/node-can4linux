var EventEmitter = require('events').EventEmitter;
var util = require('util');
var can4linux = require('./build/Release/can4linux');

var isString = function (o) {
        return (Object.prototype.toString.call(o) === '[object String]');
};

function Can(options){
    if ( !(this instanceof Can) ) 
        return new Can(options); 
    
    var can = this;
    
    EventEmitter.call(can);
    options = options || {};
    var device = options.device || "/dev/can0";
    can.self = options.self || false;
    can.sending = false;
    can.open = true;
    can.queue = [];
    can.fd = can4linux.open(device);
    if(can.fd == -1)
        throw new Error("Error opening " + device);
    
    var read = function(){
        if(open) return;
        can4linux.read(can.fd, 1000, function(err, data){
            if(err)
                can.emit('error', err);
            else
                can.emit('data', data);
            
            read();
        });
    };
};

util.inherits(Can, EventEmitter);

Can.prototype.close = function(){
    this.open = false;
    can4linux.close(this.fd);
};

Can.prototype.send = function(msg){
    can = this;
    data = {
        id: msg.id,
        ext: msg.ext || false,
        rtr: msg.rtr || false,
        data: (msg.data || []).slice()
    }
    
    if(can.sending)
        return can.queue.push(data);
    
    can.sending = true;
    
    can4linux.write(can.fd, data, function(err){
        if(err){
            can.emit('error', err);
        }
        
        var next = can.queue.shift();
        can.sending = false;
        if(next)
            can.send(next);        
    });
};

module.exports = Can;
