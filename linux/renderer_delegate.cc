// Copyright (c) 2012 The Chromium Embedded Framework Authors. All rights
// reserved. Use of this source code is governed by a BSD-style license that
// can be found in the LICENSE file.

#include "renderer_delegate.h"

#include <sstream>
#include <string>
#include <fmt/core.h>

#include "include/cef_crash_util.h"
#include "include/cef_dom.h"
#include "include/wrapper/cef_helpers.h"
#include "include/wrapper/cef_message_router.h"
#include "v8handler.h"
#include "client_renderer.h"

namespace client
{
  namespace renderer
  {
    class CopyTextVisitor : public CefDOMVisitor
    {
    public:
      CopyTextVisitor(CefRefPtr<CefFrame> frame) : frame_(frame)
      {
      }
      virtual void Visit(CefRefPtr<CefDOMDocument> document) override
      {
        CefRefPtr<CefProcessMessage> message =
            CefProcessMessage::Create(kTextSelectionReport);
        if (document->HasSelection())
        {
          message->GetArgumentList()->SetString(0, document->GetSelectionAsText());
        }
        else
        {
          message->GetArgumentList()->SetString(0, "");
        }
        frame_->SendProcessMessage(PID_BROWSER, message);
      };

    private:
      CefRefPtr<CefFrame> frame_;
      IMPLEMENT_REFCOUNTING(CopyTextVisitor);
    };
    namespace
    {

      // Must match the value in client_handler.cc.
      class ClientRenderDelegate : public ClientAppRenderer::Delegate
      {
      public:
        ClientRenderDelegate() : last_node_is_editable_(false)
        {
        }

        void OnWebKitInitialized(CefRefPtr<ClientAppRenderer> app) override
        {
          LOG(INFO) << "OnWebKitInitialized!";
          // Create the renderer-side router for query handling.
          CefMessageRouterConfig config;
          message_router_ = CefMessageRouterRendererSide::Create(config);
        }

        void OnContextCreated(CefRefPtr<ClientAppRenderer> app,
                              CefRefPtr<CefBrowser> browser,
                              CefRefPtr<CefFrame> frame,
                              CefRefPtr<CefV8Context> context) override
        {
          LOG(INFO) << "OnContextCreated!";
          // Retrieve the context's window object.
          CefRefPtr<CefV8Value> object = context->GetGlobal();

          CefRefPtr<CefV8Value> svet = CefV8Value::CreateBool(true);
          object->SetValue("isSvetophone", svet, V8_PROPERTY_ATTRIBUTE_NONE);

          if (frame->IsMain())
          {
            CefRefPtr<CefProcessMessage> message = CefProcessMessage::Create(client::renderer::kContextCreated);
            browser->GetMainFrame()->SendProcessMessage(PID_BROWSER, message);
            const auto js = fmt::format("localStorage.setItem('token', '{}'); localStorage.setItem('access_token', '{}');", token_, access_token_);
            LOG(INFO) << "executing " << js;
            browser->GetMainFrame()->ExecuteJavaScript(js, frame->GetURL(), 0);
          }

          // Create an instance of my CefV8Handler object.
          CefRefPtr<CefV8Handler> handler = new ClientV8Handler(bind_func_, [context = context](const CefString &webMessage)
                                                                {
            CefRefPtr<CefBrowser> browser = context->GetBrowser();
            if (browser && browser->GetMainFrame())
            {
                CefRefPtr<CefProcessMessage> message = CefProcessMessage::Create(client::renderer::kWebMessage);
                CefRefPtr<CefListValue> args = message->GetArgumentList();
                args->SetString(0, webMessage);
                browser->GetMainFrame()->SendProcessMessage(PID_BROWSER, message);
                return true;
            }
            else {
              return false;
            } });

          LOG(INFO) << "creating " << bind_func_ << " function handler";
          CefRefPtr<CefV8Value> func = CefV8Value::CreateFunction(bind_func_, handler);

          // Add the "myfunc" function to the "window" object.
          object->SetValue(bind_func_, func, V8_PROPERTY_ATTRIBUTE_NONE);

          // message_router_->OnContextCreated(browser, frame, context);
        }
        void OnBrowserCreated(CefRefPtr<ClientAppRenderer> app,
                              CefRefPtr<CefBrowser> browser,
                              CefRefPtr<CefDictionaryValue> extra_info) override
        {
          CefRefPtr<CefProcessMessage> message = CefProcessMessage::Create(kBrowserCreatedMessage);
          CefRefPtr<CefListValue> args = message->GetArgumentList();
          texture_id_ = extra_info->GetString("texture_id");
          bind_func_ = extra_info->GetString("bind_func");
          std::string token = extra_info->GetString("token");
          if (token != "")
          {
            token_ = token;
          }
          std::string access_token = extra_info->GetString("access_token");
          if (access_token != "")
          {
            access_token_ = access_token;
          }
          LOG(INFO) << "created browser with token " << token_ << " and access_token " << access_token_;
          args->SetString(0, texture_id_);
          CefRefPtr<CefFrame> frame = browser->GetMainFrame();
          if (frame)
          {
            frame->SendProcessMessage(PID_BROWSER, message);
          }
        }

        void OnBrowserDestroyed(CefRefPtr<ClientAppRenderer> app, CefRefPtr<CefBrowser> browser) override
        {
        }

        void OnContextReleased(CefRefPtr<ClientAppRenderer> app,
                               CefRefPtr<CefBrowser> browser,
                               CefRefPtr<CefFrame> frame,
                               CefRefPtr<CefV8Context> context) override
        {
          LOG(INFO) << "OnContextReleased!";
          // message_router_->OnContextReleased(browser, frame, context);
        }

        void OnFocusedNodeChanged(CefRefPtr<ClientAppRenderer> app,
                                  CefRefPtr<CefBrowser> browser,
                                  CefRefPtr<CefFrame> frame,
                                  CefRefPtr<CefDOMNode> node) override
        {
          bool is_editable = (node.get() && node->IsEditable());
          if (is_editable != last_node_is_editable_)
          {
            // Notify the browser of the change in focused element type.
            last_node_is_editable_ = is_editable;
            CefRefPtr<CefProcessMessage> message =
                CefProcessMessage::Create(kFocusedNodeChangedMessage);
            message->GetArgumentList()->SetBool(0, is_editable);
            frame->SendProcessMessage(PID_BROWSER, message);
          }
        }

        bool OnProcessMessageReceived(CefRefPtr<ClientAppRenderer> app,
                                      CefRefPtr<CefBrowser> browser,
                                      CefRefPtr<CefFrame> frame,
                                      CefProcessId source_process,
                                      CefRefPtr<CefProcessMessage> message) override
        {
          const std::string &message_name = message->GetName();

          LOG(INFO) << "renderer recieved " << message_name << " message";
          if (message_name == client::renderer::kTokenUpdate)
          {
            std::string new_token = message->GetArgumentList()->GetString(0).ToString();
            LOG(INFO) << "renderer update token to " << new_token;
            token_ = new_token;
            CefRefPtr<CefProcessMessage> to_browser = CefProcessMessage::Create(client::renderer::kTokenUpdate);
            browser->GetMainFrame()->SendProcessMessage(PID_BROWSER, to_browser);
          }
          if (message_name == client::renderer::kAccessTokenUpdate)
          {
            std::string new_token = message->GetArgumentList()->GetString(0).ToString();
            LOG(INFO) << "renderer update access token to " << new_token;
            access_token_ = new_token;
            CefRefPtr<CefProcessMessage> to_browser = CefProcessMessage::Create(client::renderer::kAccessTokenUpdate);
            browser->GetMainFrame()->SendProcessMessage(PID_BROWSER, to_browser);
          }
          if (message_name == client::renderer::kTextSelectionReport)
          {

            CefRefPtr<CopyTextVisitor> visitor =
                new CopyTextVisitor(frame);
            frame->VisitDOM(visitor);
          }

          // return message_router_->OnProcessMessageReceived(browser, frame,
          //                                                  source_process, message);
          return true;
        }

      private:
        bool last_node_is_editable_;
        CefString bind_func_;
        CefString texture_id_;
        std::string token_;
        std::string access_token_;

        // Handles the renderer side of query routing.
        CefRefPtr<CefMessageRouterRendererSide> message_router_;

        IMPLEMENT_REFCOUNTING(ClientRenderDelegate);
      };

    } // namespace

    void CreateDelegates(ClientAppRenderer::DelegateSet &delegates)
    {
      LOG(INFO) << "CreateDelegates!";
      delegates.insert(new ClientRenderDelegate);
    }

  } // namespace renderer
} // namespace client