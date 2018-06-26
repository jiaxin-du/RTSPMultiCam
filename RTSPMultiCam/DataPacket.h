//***********************************************************
//  RTSP server for combined picture from multiple cameras
//
//  Program written by Jiaxin Du, jiaxin.du@outlook.com
//  All right reserved.
//
//***********************************************************
#pragma once

#include "misc.h"

extern "C" {
#include <libavutil\mem.h>     //for av_mallocz and av_free
}

namespace CameraStreaming {
   class DataPacket
   {
   private:
      unsigned char*   fDataPtr;
      int              fCapacity;
      int              fSize;

      int              fRefID;

   public:

      DataPacket(const int sz = 0) :
         fDataPtr(nullptr), fCapacity(0), fSize(0), fRefID(0xFFFFFFFF)
      {
         resize(sz);
      };

      DataPacket(const DataPacket& d) :
         fDataPtr(nullptr), fCapacity(0), fSize(0)
      {
         resize(d.fSize);
         if (d.fSize != 0) {
            memcpy(fDataPtr, d.fDataPtr, d.fSize);
         }

         fRefID = d.fRefID;
      };

      DataPacket(DataPacket&& d) :
         fDataPtr(nullptr), fCapacity(0), fSize(0), fRefID(0xFFFFFFFF)
      {
         swap(d);
      };

      ~DataPacket() {
         if (fDataPtr != nullptr) {
            av_free(fDataPtr);
            fDataPtr = nullptr; 
            fCapacity = 0;
            fSize = 0;
         }
      };

      int ref_id(void) const {
         return fRefID;
      };

      void set_ref_id(int id) {
         fRefID = id;
      };

      void resize(const int& sz) {
         if (sz <0){
            wprintf("invalid data packet size");
            return;
         }
         if (sz > fCapacity) {
            if (fDataPtr != nullptr) {
               av_free(fDataPtr);  
               fCapacity = 0;
               fDataPtr = nullptr;
            }
            if (sz > 0) {
               fDataPtr = (uint8_t *)av_mallocz(sz);
               fCapacity = sz;
            }
         }
         fSize = sz;
      };

      void clear()
      {
         resize(0);
      };

      void copy(const uint8_t* buff, const int& sz) {
         if (sz <= 0) return;
         if (sz > fSize) {
            resize(sz);
         }

         memcpy(fDataPtr, buff, sz);
      };

      bool empty(void) const {
         return (fSize == 0);
      };

      int size(void) const {
         return fSize;
      };

      uint8_t* data_ptr(void) {
         return fDataPtr;
      };

      void swap(DataPacket& d) {
         std::swap(fDataPtr, d.fDataPtr);
         std::swap(fSize, d.fSize);
         std::swap(fCapacity, d.fCapacity);
         std::swap(fRefID, d.fRefID);
      };
   };
}
