#include "build.hpp"

#include <Btk/signal/trackable.hpp>

BTK_NS_BEGIN

Trackable::Trackable(){

}
Trackable::~Trackable(){
    if(_impl != nullptr){
        _impl->cleanup();
        delete _impl;
    }
}
void Trackable::Impl::cleanup(){
    // lock_guard<SpinLock> locker(spinlock);
    //cleanup all
    for(auto i = functors_cb.begin();i != functors_cb.end();){
        //Call the functor
        i->_call();
        i = functors_cb.erase(i);
    }
}
void Trackable::Impl::disconnect_all(){
    // lock_guard<SpinLock> locker(spinlock);
    //disconnect the timer
    for(auto i = functors_cb.begin();i != functors_cb.end();){
        //Call the functor
        if(i->magic == Functor::Signal){
            i->_call();
            i = functors_cb.erase(i);
        }
        else{
            ++i;
        }
    }
}
_FunctorLocation Trackable::Impl::add_callback(void(*fn)(void*),void*param){
    if(fn == nullptr){
        return {functors_cb.end()};
    }
    Functor functor;
    functor.user1 = reinterpret_cast<void*>(fn);
    functor.user2 = param;
    functor.call = [](Functor&self) -> void{
        reinterpret_cast<void(*)(void*)>(self.user1)(self.user2);
    };

    functors_cb.push_back(functor);
    return {--functors_cb.end()};
}
_FunctorLocation Trackable::Impl::add_functor(const Functor &functor){
    functors_cb.push_back(functor);
    return {--functors_cb.end()};
}
_FunctorLocation Trackable::Impl::remove_callback(FunctorLocation location){
    //Remove this callback
    if(location.iter != functors_cb.end()){
        location->_cleanup();
        functors_cb.erase(location.iter);
    }
    return {--functors_cb.end()};
}
_FunctorLocation Trackable::Impl::exec_functor(FunctorLocation location){
    //Remove this callback
    if(location.iter != functors_cb.end()){
        location->_call();
        functors_cb.erase(location.iter);
    }
    return {--functors_cb.end()};
}
_FunctorLocation Trackable::Impl::remove_callback_safe(FunctorLocation location){
    //Remove this callback after do check
    //Check is vaild location
    for(auto iter = functors_cb.begin(); iter != functors_cb.end(); ++iter){
        if(iter == location.iter){
            location->_cleanup();
            functors_cb.erase(location.iter);
            return {--functors_cb.end()};
        }
    }
    return {--functors_cb.end()};
}
// void Trackable::Impl::dump_functors(FILE *output) const{
//     if(output == nullptr){
//         output = stderr;
//     }
//     lock_guard<SpinLock> locker(spinlock);
//     for(auto &f:functors_cb){
//         const char *type = nullptr;
//         switch(f.magic){
//             case Functor::Signal: type = "Signal";break;
//             case Functor::Unknown: type = "Unknown";break;
//             case Functor::Timer: type = "Timer";break;
//         }
//         fprintf(output,"  -(%s) => %p\n",type,f.call);
//     }
// }
// Uint32 Trackable::GetTicks(){
//     return SDL_GetTicks();
// }
SignalBase::SignalBase() {

}
SignalBase::~SignalBase() {
    disconnect_all();
}
void SignalBase::disconnect_all(){
    auto iter = slots.begin();
    while(iter != slots.end()){
        (*iter)->cleanup();
        iter = slots.erase(iter);
    }
}
void Connection::disconnect(bool from_object){
    if(status == WithSignal){
        (*sig.iter)->cleanup(from_object);
        sig.current->slots.erase(sig.iter);
    }
    else if(status == WithObject){
        obj.object->exec_functor(obj.loc);
    }
    status = None;
}
auto Trackable::impl() const -> Impl&{
    if(_impl == nullptr){
        _impl = new Impl;
    }
    return *_impl;
}
//Remove the 
_ConnectionFunctor::_ConnectionFunctor(Connection con){
    //Execute the functor
    call = [](_Functor &self){
        auto *con = static_cast<Connection*>(self.user1);
        con->disconnect(true);
        delete con;
    };
    //Just cleanup
    cleanup = [](_Functor &self){
        auto *con = static_cast<Connection*>(self.user1);
        delete con;
    };

    user1 = new Connection(con);
    magic = Signal;
}
_GenericCallFunctor::_GenericCallFunctor(_GenericCallBase *base){
    call = [](_Functor &self){
        static_cast<_GenericCallBase*>(self.user1)->deleted = true;
    };
    cleanup = nullptr;
    user1 = base;
    magic = Unknown;
}

BTK_NS_END