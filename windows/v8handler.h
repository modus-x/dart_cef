#include "include/cef_v8.h"
#include "browser.h"
#include "include/base/cef_ref_counted.h"
#include "renderer_delegate.h"

class ClientV8Handler : public CefV8Handler
{
public:
    ClientV8Handler(const CefString &bindFunc, client::renderer::WebMessageCallback callback) : bindFunc_(bindFunc), callback_(callback)
    {
    }

    virtual bool Execute(const CefString &name,
                         CefRefPtr<CefV8Value> object,
                         const CefV8ValueList &arguments,
                         CefRefPtr<CefV8Value> &retval,
                         CefString &exception) override
    {
        if (name == bindFunc_)
        {
            CefString webMessage = arguments.at(0)->GetStringValue();
            bool result = callback_(webMessage);
            retval = CefV8Value::CreateBool(result);
            return true;
        }

        // Function does not exist.
        return false;
    }

private:
    CefString bindFunc_;
    client::renderer::WebMessageCallback callback_;

    // Provide the reference counting implementation for this class.
    IMPLEMENT_REFCOUNTING(ClientV8Handler);
};