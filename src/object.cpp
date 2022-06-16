#include "build.hpp"

#include <Btk/context.hpp>
#include <Btk/object.hpp>
#include <vector>
#include <set>

BTK_NS_BEGIN

struct UserData {
    UserData *next = nullptr;
    Any *value = nullptr;
    u8string key;
};

//< Object implementation
struct ObjectImpl {
    UserData *chain = nullptr;
    UIContext *ctxt = nullptr;

    
    ~ObjectImpl() {
        UserData *cur = chain;
        while (cur) {
            UserData *next = cur->next;
            delete cur->value;
            delete cur;
            cur = next;
        }

    }
    void set_userdata(const char_t *key, Any *value) {
        UserData *cur = chain;
        while (cur) {
            if (cur->key == key) {
                cur->value = value;
                return;
            }
            cur = cur->next;
        }
        UserData *new_data = new UserData;
        new_data->key = key;
        new_data->value = value;
        new_data->next = chain;
        chain = new_data;
    }
    Any *userdata(const char_t *key) const {
        UserData *cur = chain;
        while (cur) {
            if (cur->key == key) {
                return cur->value;
            }
            cur = cur->next;
        }
        return nullptr;
    }
};

Object::~Object() {
    delete priv;
}
void Object::set_userdata(const char_t *key, Any *value) {
    implment()->set_userdata(key, value);
}
Any *Object::userdata(const char_t *key) const {
    return implment()->userdata(key);
}
bool Object::handle(Event &event) {
    return false;
}

ObjectImpl *Object::implment() const {
    if (priv == nullptr) {
        priv = new ObjectImpl;
        priv->ctxt = GetUIContext();
        BTK_ASSERT_MSG(priv->ctxt,"UIContext couldnot be nullptr");
    }
    return priv;
}
UIContext  *Object::ui_context() const {
    return implment()->ctxt;
}

BTK_NS_END