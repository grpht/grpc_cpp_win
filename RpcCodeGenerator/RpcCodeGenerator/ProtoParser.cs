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
        public static RpcService ParseProtoFile(string protoContent)
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

                var rpcMethodMatches = Regex.Matches(trimmedLine, @"rpc\s+(\w+)\s*\(\s*(stream\s+)?(\w+)\s*\)\s+returns\s+\(\s*(stream\s+)?(\w+)\s*\)\s*\{?\s*\}?");
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

        public static string GenerateServerRpc(RpcService service)
        {
            string className = service.name;
            bool bNamespace = !String.IsNullOrEmpty(service.package);
            StringBuilder builder = new StringBuilder();
            builder.AppendLine("/* Auto-Generated File */");
            builder.AppendLine(TextFormat.serverIncludeTxt);
            builder.AppendLine($"#include models/\"{service.fileName}.grpc.pb.h\"");
            foreach (string im in service.imports)
            {
                builder.AppendLine($"#include models/\"{im}.grpc.pb.h\"");
            }
            if (bNamespace) builder.AppendLine($"namespace {service.package} \n{{");
            builder.AppendLine($"  class {className}Service : public {className}::CallbackService, public RpcService");
            builder.AppendLine("  {");
            builder.AppendLine($"  protected:");
            builder.AppendLine($"    virtual {className}Service* GetInstance() = 0;");
            builder.AppendLine($"  public:");
            builder.AppendLine(CreateMacro(4, "SERVER", service.methods));
            builder.AppendLine("  };");
            if (bNamespace) builder.AppendLine("}");

            return builder.ToString();
        }

        public static string GenerateClientRpc(RpcService service)
        {
            string className = service.name;
            bool bNamespace = !String.IsNullOrEmpty(service.package);

            StringBuilder builder = new StringBuilder();
            builder.AppendLine("/* Auto-Generated File */");
            builder.AppendLine(TextFormat.clientIncludeTxt);
            builder.AppendLine($"#include models/\"{service.fileName}.grpc.pb.h\"");
            foreach (string im in service.imports)
            {
                builder.AppendLine($"#include models/\"{im}.grpc.pb.h\"");
            }
            if (bNamespace) builder.AppendLine($"namespace {service.package} \n{{");
            builder.AppendLine($"  class {className}ServiceClient : public RpcServiceClient");
            builder.AppendLine("  {");
            builder.AppendLine($"  protected:");
            builder.AppendLine($"    virtual {className}ServiceClient* GetInstance() = 0;");
            builder.AppendLine($"    void InitStub(std::shared_ptr<grpc::Channel> channel) override {{ _stub = {className}::NewStub(channel); }}");
            builder.AppendLine($"    std::unique_ptr<{className}::Stub> _stub;");
            builder.AppendLine($"  public:");
            builder.AppendLine(CreateMacro(4, $"CLIENT", service.methods));
            builder.AppendLine("  };");
            if (bNamespace) builder.AppendLine("}");

            return builder.ToString();
        }


        private static string CreateMacro(int tabSize, string macroName, List<RpcMethod> methods)
        {
            string macro = string.Empty;
            string indent = new string(' ', tabSize);
            foreach (var method in methods)
            {
                if (method.requestStream && method.responseStream) // BiStream
                {
                    macro += $"{indent}{macroName}_BISTREAM({method.name}, {method.request}, {method.response})\n";
                }
                else if (method.requestStream) //ClientStream
                {
                    macro += $"{indent}{macroName}_CSTREAM({method.name}, {method.request}, {method.response})\n";
                }
                else if (method.responseStream) // ServerStream
                {
                    macro += $"{indent}{macroName}_SSTREAM({method.name}, {method.request}, {method.response})\n";
                }
                else //Unary
                {
                    macro += $"{indent}{macroName}_UNARY({method.name}, {method.request}, {method.response})\n";
                }
            }
            return macro;
        }
    }
}
