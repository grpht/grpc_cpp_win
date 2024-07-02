using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading.Tasks;

namespace RpcCodeGenerator
{
    public class FileMover
    {
        public static void MoveFiles(string sourceFolderPath, string destinationFolderPath, string requireKeyword)
        {
            if (!Directory.Exists(sourceFolderPath))
            {
                throw new DirectoryNotFoundException($"Source folder not found: {sourceFolderPath}");
            }

            if (!Directory.Exists(destinationFolderPath))
            {
                Directory.CreateDirectory(destinationFolderPath);
            }

            string[] files = Directory.GetFiles(sourceFolderPath);

            foreach (string file in files)
            {
                string fileName = Path.GetFileName(file);

                if (fileName.Contains(requireKeyword))
                {
                    string destFilePath = Path.Combine(destinationFolderPath, fileName);

                    File.Move(file, destFilePath, overwrite: true);
                    Console.WriteLine($"Moved file: {fileName}");
                }
            }

            Console.WriteLine("Files moved successfully.");
        }
    }
}
