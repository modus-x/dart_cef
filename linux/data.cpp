#include "include/cef_parser.h"

std::string GetDataURI(const std::string &data, const std::string &mime_type)
{
  return "data:" + mime_type + ";base64," +
         CefURIEncode(CefBase64Encode(data.data(), data.size()), false)
             .ToString();

}