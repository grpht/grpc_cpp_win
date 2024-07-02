using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Text.RegularExpressions;
using System.Threading.Tasks;

namespace RpcCodeGenerator
{
    public class ProtoParser
    {
        public string FileName { get; set; }

        public RpcService ParseProtoFile(string protoContent)
        {
            RpcService service = new RpcService();

            var liens = protoContent.Split(new[] { '\r', '\n' }, StringSplitOptions.RemoveEmptyEntries);
            bool inBlockComment = false;
            foreach (var line in liens)
            {
                string trimmedLine = line.Trim();

                // 주석 건너뛰기 start
                if (inBlockComment)
                {
                    if (trimmedLine.Contains("*/"))
                        inBlockComment = false;
                    continue;
                }

                if (trimmedLine.StartsWith("/*"))
                {
                    if (!trimmedLine.Contains("*/"))
                    {
                        inBlockComment = true;
                    }
                    continue;
                }

                if (trimmedLine.StartsWith("//") || trimmedLine.StartsWith("/*") || trimmedLine.StartsWith("*"))
                {
                    continue;
                }
                // 주석 건너뛰기 end


                var packageMatch = Regex.Match(trimmedLine, @"package\s+(\w+);");
                if (packageMatch.Success)
                {
                    service.package = packageMatch.Groups[1].Value;
                    continue;
                }

                var importMatches = Regex.Matches(trimmedLine, @"import\s+""(\w*).proto""");
                foreach (Match match in importMatches)
                {
                    service.imports.Add(match.Groups[1].Value);
                    continue;
                }


                var serviceMatch = Regex.Match(trimmedLine, @"service\s+(\w+)\s*\{");
                if (serviceMatch.Success)
                {
                    service.exist = true;
                    service.name = serviceMatch.Groups[1].Value;
                    continue;
                }

                var rpcMethodMatches = Regex.Matches(trimmedLine, @"rpc\s+(\w+)\s*\((stream\s+)?(\w+)\)\s+returns\s+\((stream\s+)?(\w+)\)\s*\{?\s*\}?");
                foreach (Match match in rpcMethodMatches)
                {
                    RpcMethod method = new RpcMethod
                    {
                        name = match.Groups[1].Value,
                        requestStream = match.Groups[2].Success,
                        request = match.Groups[3].Value,
                        responseStream = match.Groups[4].Success,
                        response = match.Groups[5].Value
                    };

                    service.methods.Add(method);
                }
            }

            return service;
        }

        public string GenerateCppClass(RpcService service, string env)
        {
            string className = service.name;

            StringBuilder builder = new StringBuilder();
            builder.Append(TextFormat.includeTxt);
            builder.AppendLine($"#include \"{FileName}.grpc.pb.h\"");
            foreach (string im in service.imports)
            {
                builder.AppendLine($"#include \"{im}.grpc.pb.h\"");
            }
            builder.AppendLine($"namespace {service.package} \n{{");
            builder.AppendLine(CreateMacro($"USING_{env}", service.methods, 2));
            builder.AppendLine($"  class {className}Service : public {className}::CallbackService, public RpcService");
            builder.AppendLine($"  {{\n  public:");
            if (env == "SERVER")
            {
                builder.AppendLine("    class Client {");
                builder.AppendLine(CreateMacro($"REQ", service.methods, 6));
                builder.AppendLine("    };");
                builder.AppendLine("    std::unordered_map<std::string, Client> _clients;");
            }
            builder.AppendLine(CreateMacro($"METHOD_{env}", service.methods));
            builder.AppendLine("  };\n}");

            return builder.ToString();
        }

        private string CreateMacro(string macroName, List<RpcMethod> methods, int tabSize = 4)
        {
            string macro = string.Empty;
            string indent = new string(' ', tabSize);
            foreach (var method in methods)
            {
                if (method.requestStream && method.responseStream) // BiStream
                    macro += $"{indent}{macroName}_BISTREAM({method.name}, {method.request}, {method.response})\n";
                else if (method.requestStream) //ClientStream
                    macro += $"{indent}{macroName}_CSTREAM({method.name}, {method.request}, {method.response})\n";
                else if (method.responseStream) // ServerStream
                    macro += $"{indent}{macroName}_SSTREAM({method.name}, {method.request}, {method.response})\n";
                else //Unary
                    macro += $"{indent}{macroName}_UNARY({method.name}, {method.request}, {method.response})\n";
            }
            return macro;
        }
    }
}
