using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading.Tasks;

namespace RpcCodeGenerator
{
    internal class TextFormat
    {
        public static string includeTxt =
@"#pragma once
#pragma warning(disable :4251)
#pragma warning(disable :4819)

#include <iostream>
#include <memory>
#include <string>
#include <unordered_map>
#include <tuple>
#include <thread>
#include <chrono>

#include ""RpcService.h""
#include ""RpcTemplate.h""

#include ""greeter.grpc.pb.h""

";
    }
}
