#pragma once

#include <DriverSpecs.h>

__user_driver

#include <sal.h>
#include <windows.h>
#include <cstring>
#include <intsafe.h>
#include <new>
#include <memory>
#include <vector>
#include <objbase.h>
#include <oleauto.h>
#include <winspool.h>
#include <filterpipeline.h>
#include <filterpipelineutil.h>
#include <prntvpt.h>
#include <atlbase.h>
#include <wincodec.h>
#include <msxml6.h>
#include <msopc.h>
#include <XpsObjectModel.h>
#include <xpsrassvc.h>

//
// Macros.
//
#define INITIALIZE_HTTP_RESPONSE( resp, status, reason )    \
    do                                                      \
    {                                                       \
        RtlZeroMemory( (resp), sizeof(*(resp)) );           \
        (resp)->StatusCode = (status);                      \
        (resp)->pReason = (reason);                         \
        (resp)->ReasonLength = (USHORT) strlen(reason);     \
    } while (FALSE)

#define ADD_KNOWN_HEADER(Response, HeaderId, RawValue)                \
    do                                                                \
    {                                                                 \
        (Response).Headers.KnownHeaders[(HeaderId)].pRawValue =       \
                                                          (RawValue); \
        (Response).Headers.KnownHeaders[(HeaderId)].RawValueLength =  \
            (USHORT) strlen(RawValue);                                \
    } while(FALSE)

#define ALLOC_MEM(cb) HeapAlloc(GetProcessHeap(), 0, (cb))

#define FREE_MEM(ptr) HeapFree(GetProcessHeap(), 0, (ptr))

class OnvifServer
{
private:
      HANDLE                     fhReqQueue = NULL;
      std::vector<std::string>   fsURLs;
public:
   OnvifServer();
   ~OnvifServer();

   DWORD DoReceiveRequests();
};

