/*  This is a test program demonstrating and validating the functionality of riban Framebuffer Library
    Copyright riban 2019
    Author: Brian Walton brian@riban.co.uk
*/

#include "ribanfblib.h"
#include <cstdio>
#include <unistd.h>

int main(int argc, char* argv[])
{
    ribanfblib fb("/dev/fb1");
    fb.Clear();
    fb.DrawText("riban", 10, 10, GREEN);
    return 0;
}
