#include <nan.h>
#include <sstream>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include "can4linux.h"      // NOLINT(build/include)

using namespace std;
using namespace Nan;

#define key(a) (Nan::New(a).ToLocalChecked())
#define flag(value, flag) (New((value & flag) ? true : false))
#define asInt(value) To<int>(value).FromJust()
#define maybeAsInt(value) asInt(value.ToLocalChecked())

#ifdef WIN32
    #include <io.h>
    #pragma comment (lib, "Ws2_32.lib")
	#define FRAME_SIZE sizeof(canmsg_t)
	#define O_NONBLOCK 0
	
#else
	#define FRAME_SIZE 1
#endif

class WriteWorker : public AsyncWorker {
    public:
		WriteWorker(Callback *callback, int fd, canmsg_t tx)
            : AsyncWorker(callback), fd(fd), tx(tx) {}
		~WriteWorker() {}

        void Execute () {
            int sent= write(fd, &tx, FRAME_SIZE);
            if (sent != FRAME_SIZE) {
                if (sent==-1){
                    stringstream ss;
                    ss << "Send failed (error number " << errno << ")";
                    SetErrorMessage(ss.str().c_str());
                }
                else
                    SetErrorMessage("Number of sent frames not 1");
            }
        }

    private:
        int fd;
        canmsg_t tx;
};

class ReadWorker : public AsyncWorker {
public:
	ReadWorker(Callback *callback, int fd, int timeout)
		: AsyncWorker(callback), fd(fd), timeout(timeout){}
	~ReadWorker() {}

	void Execute() {
		fd_set   rfds;
		struct   timeval tv;
		int      rv;

		FD_ZERO(&rfds);
		FD_SET(fd, &rfds);

		/* wait some time  before process terminates, if timeout > 0 */
		tv.tv_sec = 0;
		tv.tv_usec = timeout;

		rv = select(fd + 1, &rfds, NULL, NULL, (timeout >= 0 ? &tv : NULL));

		if (rv == -1) {
			stringstream ss;
			ss << "Read failed (error number " << errno << ")";
			SetErrorMessage(ss.str().c_str());
			return;
		}

		if (rv == 0 || !FD_ISSET(fd, &rfds))
			return;

		rv = read(fd, &rx, FRAME_SIZE);
		rv = 1;

		if (rv == 1 && rx.id != CANDRIVERERROR)
			return;

		if (rv == -1){
			stringstream ss;
			ss << "Read failed (error number " << errno << ")";
			SetErrorMessage(ss.str().c_str());
			return;
		}

		if (rv != 1){
			SetErrorMessage("Invalid length of received frame");
			return;
		}

		if (rx.flags & MSG_OVR){
			SetErrorMessage("Message overflow");
			return;
		}

		if (rx.flags & MSG_PASSIVE){
			SetErrorMessage("Controller passive");
			return;
		}

		if (rx.flags & MSG_BUSOFF){
			SetErrorMessage("Controller bus off");
			return;
		}

		if (rx.flags & MSG_WARNING){
			SetErrorMessage("Warning level reached");
			return;
		}

		if (rx.flags & MSG_BOVR){
			SetErrorMessage("Receive/transmit buffer overflow");
			return;
		}
	}

	void HandleOKCallback(){
		HandleScope scope;

		v8::Local<v8::Object> obj = New<v8::Object>();
		Set(obj, key("id"), New(rx.id));
		Set(obj, key("rtr"), flag(rx.flags, MSG_RTR));
		Set(obj, key("self"), flag(rx.flags, MSG_SELF));
		Set(obj, key("ext"), flag(rx.flags, MSG_EXT));

		v8::Local<v8::Object> timestamp = New<v8::Object>();
		Set(timestamp, key("sec"), New(rx.timestamp.tv_sec));
		Set(timestamp, key("usec"), New(rx.timestamp.tv_usec));
		Set(obj, key("timestamp"), timestamp);

		v8::Local<v8::Array> arr = New<v8::Array>(rx.length);
		for (unsigned int i = 0; i < rx.length; i++){
			Set(arr, i, New(rx.data[i]));
		}

		v8::Local<v8::Value> argv[] = { Null(), obj };

		callback->Call(2, argv);
	}

private:
	int fd;
	int timeout;
	canmsg_t rx;
};

NAN_METHOD(canOpen) {
    Utf8String *s = new Utf8String(info[0]);
	int fd = open(s->operator*(), O_RDWR | O_NONBLOCK);
    printf("File path: %s", s->operator*());
    info.GetReturnValue().Set(fd);
}

NAN_METHOD(canClose) {
	int fd = asInt(info[0]);
	close(fd);
}

NAN_METHOD(canRead) {
  int fd = asInt(info[0]);
  int timeout = To<int>(info[1]).FromJust();
  Callback *callback = new Callback(info[2].As<v8::Function>());

  AsyncQueueWorker(new ReadWorker(callback, fd, timeout));
}

NAN_METHOD(canWrite) {
	int fd = asInt(info[0]);
	v8::Local<v8::Object> obj = info[1].As<v8::Object>();

	Callback *callback = new Callback(info[2].As<v8::Function>());

	canmsg_t tx;
	memset(&tx, 0, sizeof(tx));

	tx.id = maybeAsInt(Get(obj, key("id")));
	v8::Local<v8::Array> arr = Get(obj, key("data")).ToLocalChecked().As<v8::Array>();

	tx.length = arr->Length();
	if (tx.length > CAN_MSG_LENGTH)
		tx.length = CAN_MSG_LENGTH;

	for (unsigned int i = 0; i < tx.length && i < CAN_MSG_LENGTH; i++){
		tx.data[i] = maybeAsInt(Get(arr, i));
	}

	bool ext = To<bool>(Get(obj, key("ext")).ToLocalChecked()).FromJust();
	if (To<bool>(Get(obj, key("rtr")).ToLocalChecked()).FromJust())
		tx.flags |= MSG_RTR;

	if (ext){
		tx.id &= CAN_EFF_MASK;
		tx.flags |= MSG_EXT;
	}
	else
		tx.id &= CAN_SFF_MASK;

	AsyncQueueWorker(new WriteWorker(callback, fd, tx));
}

NAN_MODULE_INIT(InitAll) {
	Set(target, New<v8::String>("open").ToLocalChecked(),
    GetFunction(New<v8::FunctionTemplate>(canOpen)).ToLocalChecked());

	Set(target, New<v8::String>("read").ToLocalChecked(),
	  GetFunction(New<v8::FunctionTemplate>(canRead)).ToLocalChecked());
    
	Set(target, New<v8::String>("write").ToLocalChecked(),
	  GetFunction(New<v8::FunctionTemplate>(canWrite)).ToLocalChecked());
    
	Set(target, New<v8::String>("close").ToLocalChecked(),
	  GetFunction(New<v8::FunctionTemplate>(canClose)).ToLocalChecked());
}

NODE_MODULE(can4linux, InitAll)