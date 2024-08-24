// See https://aka.ms/new-console-template for more information
using RpcCodeGenerator;
using System.Text.RegularExpressions;
string executePath = @"D:\DSR\DSR_Server\grpc\";
string protoPath = $"{executePath}protos";
string cppOutputPath = $"{executePath}src\\services";
string modelPath = $"{executePath}\\src\\models";

List<RpcService> services = new List<RpcService>();
if (args.Length != 0)
{
    executePath = args[0];
}

//parse Service
string[] protoFiles = Directory.GetFiles(protoPath, "*.proto");

foreach (var filePath in protoFiles)
{
    string protoContent = File.ReadAllText(filePath);

    RpcService rpcService = ProtoParser.ParseProtoFile(protoContent);
    if (rpcService.exist)
    {
        rpcService.fileName = Path.GetFileNameWithoutExtension(filePath);
        services.Add(rpcService);
        string serverRpc = NormalizeLineEndings(ProtoParser.GenerateServerRpc(rpcService));
        Console.WriteLine(serverRpc);
        File.WriteAllText(Path.Combine(cppOutputPath, $"{rpcService.name}Service.h"), serverRpc);

        string clientRpc = NormalizeLineEndings(ProtoParser.GenerateClientRpc(rpcService));
        Console.WriteLine(clientRpc);
        File.WriteAllText(Path.Combine(cppOutputPath, $"{rpcService.name}ServiceClient.h"), clientRpc);
    }
}

//Add #include 'stdafx.h' to .cc file
{
    try
    {
        string[] files = Directory.GetFiles(modelPath, "*.cc");
        foreach (string file in files)
        {
            string tempFile = Path.GetTempFileName();
            bool hasStdafx = false;

            using (var sr = new StreamReader(file))
            {
                if (!sr.EndOfStream)
                {
                    string firstLine = sr.ReadLine();
                    hasStdafx = firstLine.Contains("#include \"stdafx.h\"");
                    if (hasStdafx)
                    {
                        // 이미 stdafx.h가 존재하는 경우 그대로 복사
                        using (var sw = new StreamWriter(tempFile))
                        {
                            sw.WriteLine(firstLine); // 첫 줄 복사
                            while (!sr.EndOfStream)
                            {
                                string line = sr.ReadLine();
                                sw.WriteLine(line);
                            }
                        }
                    }
                    else
                    {
                        // stdafx.h가 없는 경우 추가
                        using (var sw = new StreamWriter(tempFile))
                        {
                            sw.WriteLine("#include \"stdafx.h\"");
                            sw.WriteLine(firstLine); // 첫 줄 복사
                            while (!sr.EndOfStream)
                            {
                                string line = sr.ReadLine();
                                sw.WriteLine(line);
                            }
                        }
                    }
                }
            }

            tempFile = NormalizeLineEndings(tempFile);
            File.Delete(file);
            File.Move(tempFile, file);
        }
    }
    catch (Exception e)
    {
        Console.WriteLine(e);
        Console.WriteLine("stdafx.h 추가 실패");
        Console.ReadLine();
        return;
    }
    Console.WriteLine("stdafx.h 추가 성공");
}

string NormalizeLineEndings(string input)
{
    // 모든 줄 끝을 \r\n (CR LF)로 변환
    return Regex.Replace(input, @"\r\n|\n|\r", "\r\n");
}