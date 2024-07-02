using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading.Tasks;

namespace RpcCodeGenerator
{
    public class RpcMethod
    {
        public string name = "";
        public string request = "";
        public string response = "";
        public bool requestStream = false;
        public bool responseStream = false;
    }

    public class RpcService
    {
        public bool exist = false;
        public string name = "";
        public string package = "";
        public List<string> imports = new List<string>();
        public List<RpcMethod> methods = new List<RpcMethod>();
    }
}
