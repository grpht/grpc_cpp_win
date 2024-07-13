// See https://aka.ms/new-console-template for more information
using RpcCodeGenerator;
string executePath = @"D:\DSR\DSR_Server\grpc\";
string protoPath = $"{executePath}protos";
string cppOutputPath = $"{executePath}src\\services";

List<RpcService> services = new List<RpcService>();
if (args.Length != 0)
{
    executePath = args[0];
}

string[] protoFiles = Directory.GetFiles(protoPath, "*.proto");

foreach (var filePath in protoFiles)
{
    string protoContent = File.ReadAllText(filePath);

    RpcService rpcService = ProtoParser.ParseProtoFile(protoContent);
    if (rpcService.exist)
    {
        rpcService.fileName = Path.GetFileNameWithoutExtension(filePath);
        services.Add(rpcService);
        string serverRpc = ProtoParser.GenerateServerRpc(rpcService);
        Console.WriteLine(serverRpc);
        File.WriteAllText(Path.Combine(cppOutputPath, $"{rpcService.name}Service.h"), serverRpc);

        string clientRpc = ProtoParser.GenerateClientRpc(rpcService);
        Console.WriteLine(clientRpc);
        File.WriteAllText(Path.Combine(cppOutputPath, $"{rpcService.name}ServiceClient.h"), clientRpc);
    }
}
