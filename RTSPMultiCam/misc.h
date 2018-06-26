//***********************************************************
//  RTSP server for combined picture from multiple cameras
//
//  Program written by Jiaxin Du, jiaxin.du@outlook.com
//  All right reserved.
//
//***********************************************************
#pragma once

#include "defines.h"

#define   LETTER_WIDTH_PIXEL    (8)
#define   LETTER_HEIGHT_PIXEL   (15)

extern unsigned char pixel_array_letters[95][15];

namespace CameraStreaming {
   class TParamPair {
   public:
      TParamPair(const std::string& xname, const std::string& xval) :
         name(xname), val(xval)
      { };

      ~TParamPair() = default;

      std::string name;

      std::string val;
   };
};

std::string byte2str(const uint64_t&);

///convert a float number to a string
std::string double2str(const double&, const char* format = "%g");

///convert a integer number to a string
std::string int2str(const int&, const int& digit = 0);

bool str2byte(const std::string&, uint64_t &);

bool str2double(const std::string&, double &);

bool str2int(const std::string&, int &);

std::string remove_comments(const std::string &);

std::string strtrim(const std::string &);

void strsplit(const std::string&, const std::string&, std::vector<std::string>&);

bool read_param(const char*, std::map <std::string, std::string> &);

void dump_binary(const uint8_t *, int);

inline void print_red_text(FILE* fp, const char* str)
{
   CONSOLE_SCREEN_BUFFER_INFO Info;
   GetConsoleScreenBufferInfo(GetStdHandle(STD_OUTPUT_HANDLE), &Info);
   SetConsoleTextAttribute(GetStdHandle(STD_OUTPUT_HANDLE), FOREGROUND_INTENSITY | FOREGROUND_RED);
   fprintf(fp, "%s", str);
   SetConsoleTextAttribute(GetStdHandle(STD_OUTPUT_HANDLE), Info.wAttributes);
};

inline void print_blue_text(FILE* fp, const char* str)
{
   CONSOLE_SCREEN_BUFFER_INFO Info;
   GetConsoleScreenBufferInfo(GetStdHandle(STD_OUTPUT_HANDLE), &Info);
   SetConsoleTextAttribute(GetStdHandle(STD_OUTPUT_HANDLE), FOREGROUND_INTENSITY | FOREGROUND_BLUE);
   fprintf(fp, "%s", str);
   SetConsoleTextAttribute(GetStdHandle(STD_OUTPUT_HANDLE), Info.wAttributes);
};

inline void print_green_text(FILE* fp, const char* str)
{
   CONSOLE_SCREEN_BUFFER_INFO Info;
   GetConsoleScreenBufferInfo(GetStdHandle(STD_OUTPUT_HANDLE), &Info);
   SetConsoleTextAttribute(GetStdHandle(STD_OUTPUT_HANDLE), FOREGROUND_INTENSITY | FOREGROUND_GREEN);
   fprintf(fp, "%s", str);
   SetConsoleTextAttribute(GetStdHandle(STD_OUTPUT_HANDLE), Info.wAttributes);
};

#define eprintf(...) do { print_red_text (stderr, "[Error] "); \
                         fprintf (stderr, __VA_ARGS__); \
                         fprintf (stderr, " ([%s]: L%d in '%s')\n", __FUNCTION__, __LINE__, __FILE__); \
                       } while(0)

#define wprintf(...) do { print_blue_text (stderr, "[Warning] "); \
                         fprintf (stderr, __VA_ARGS__); \
                         fprintf (stderr, " ([%s]: L%d in '%s')\n", __FUNCTION__, __LINE__, __FILE__); \
                       } while(0)

#define iprintf(...) do { print_green_text (stdout, "[Info] "); \
                         fprintf (stdout, __VA_ARGS__); \
                         fprintf (stdout, "\n"); \
                       } while(0)

