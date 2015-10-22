// =============================================================================
//
// Copyright (c) 2014 Christopher Baker <http://christopherbaker.net>
//
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included in
// all copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
// THE SOFTWARE.
//
// =============================================================================


#include "EventLoggerChannel.h"


EventLoggerChannel::EventLoggerChannel()
{
}


EventLoggerChannel::~EventLoggerChannel()
{
}


void EventLoggerChannel::log(ofLogLevel level,
                             const std::string& module,
                             const std::string& message)
{
    ofConsoleLoggerChannel::log(level, module, message);

    LoggerEventArgs args;
    args.level = level;
    args.module = module;
    args.message = message;
    args.timestamp = std::chrono::system_clock::now();

    event.notify(this, args);
}


void EventLoggerChannel::log(ofLogLevel level,
                             const std::string& module,
                             const char* format, ...)
{
    va_list args;
    va_start(args, format);
    log(level, module, format, args);
    va_end(args);
}

void EventLoggerChannel::log(ofLogLevel level,
                             const std::string& module,
                             const char* format,
                             va_list args)
{
    ofConsoleLoggerChannel::log(level, module, format, args);
}
