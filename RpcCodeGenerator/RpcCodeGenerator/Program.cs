// See https://aka.ms/new-console-template for more information
using RpcCodeGenerator;

Console.WriteLine("Hello, World!");

string protoPath = @"";
string cppOutputPath = @"";
List<RpcService> services = new List<RpcService>();

string[] protoFiles = Directory.GetFiles(protoPath, "*.proto");
ProtoParser parser = new ProtoParser();
foreach (var filePath in protoFiles)
{
    parser.FileName = Path.GetFileNameWithoutExtension(filePath);
    string protoContent = File.ReadAllText(filePath);

    RpcService rpcService = parser.ParseProtoFile(protoContent);
    if (rpcService.exist)
    {
        services.Add(rpcService);
        string serverTxt = parser.GenerateCppClass(rpcService, "SERVER");
        File.WriteAllText(Path.Combine(cppOutputPath, $"{rpcService.name}Service.h"), serverTxt);

        string clientTxt = parser.GenerateCppClass(rpcService, "SERVER");
        File.WriteAllText(Path.Combine(cppOutputPath, $"{rpcService.name}ServiceClient.h"), clientTxt);
    }
}