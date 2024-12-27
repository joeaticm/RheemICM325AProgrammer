using Newtonsoft.Json;
using System;
using System.Collections.Generic;
using System.IO;
using System.Linq;
using System.Text;
using System.Threading.Tasks;

namespace RheemICM325AProgrammer
{
    internal class Configuration
    {
        public string OutputDirectory { get; set; } = ".";
        public bool CheckBarcode { get; set; } = false;
        public List<ICM325A> Models { get; set; } = [];
    }
}
