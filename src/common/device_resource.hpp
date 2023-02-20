#include "build.hpp"

#include <Btk/detail/reference.hpp>
#include <Btk/detail/device.hpp>
#include <Btk/detail/types.hpp>
#include <Btk/object.hpp>
#include <map>

BTK_NS_BEGIN

/**
 * @brief Manager for manage the device resources
 * 
 */
class PaintResourceManager : public Trackable {
    public:
        PaintResourceManager() { }
        PaintResourceManager(const PaintResourceManager &m) { } //< Called on Copy on write
        ~PaintResourceManager() { reset_manager(); }
        
        void add_resource(void *context, PaintResource *resource) {
            auto con = resource->signal_destroyed().connect(
                Bind(&PaintResourceManager::on_context_destroyed, this, context)
            );
            BTK_LOG(
                "[PaintResourceManager::add_resource] add(%p, %p : %s)\n", 
                context, 
                resource, 
                Btk_typename(resource)
            );

            resources.emplace(context, std::make_pair(con, resource));
        }

        template <typename T = PaintResource>
        T   *get_resource(void *context) {
            auto iter = resources.find(context);
            if (iter == resources.end()) {
                return nullptr;
            }
            return static_cast<T *>(iter->second.second.get());
        }

        void reset_manager() {
            for (auto &p : resources) {
                // Disconnect all connections
                p.second.first.disconnect();
                BTK_LOG(
                    "[PaintResourceManager::reset_manager] del(%p, %p : %s)\n", 
                    p.first, 
                    p.second.second.get(), 
                    Btk_typename(p.second.second.get())
                );
            }
            resources.clear();
        }
    private:
        void on_context_destroyed(void *ctxt) {
            auto iter = resources.find(ctxt);
            if (iter != resources.end()) {
                BTK_LOG(
                    "[PaintResourceManager::on_context_destroyed] del(%p, %p : %s)\n", 
                    iter->first, 
                    iter->second.second.get(), 
                    Btk_typename(iter->second.second.get())
                );
                // Disconnect connection from 
                iter->second.first.disconnect();
                resources.erase(iter);
            }
        }

        // Map Key => Device resources, with painter connection
        std::map<
            void *, 
            std::pair<Connection, Ref<PaintResource> >
        > resources;
};

BTK_NS_END