using System;
using System.Collections.Generic;
using System.IO.Ports;
using System.Linq;
using System.Management;
using System.Printing.IndexedProperties;
using System.Reflection.Metadata.Ecma335;
using System.Text;
using System.Threading.Tasks;
using System.Windows.Controls;

namespace RheemICM325AProgrammer
{
    public class ICM325A
    {
        // False - Temperature True - Pressure
        public bool ProbeType { get; set; }
        // 50-500
        public ushort SetPoint { get; set; }
        // 1-50
        public byte HardStart { get; set; }
        // 17-48
        public byte MinimumOutputVoltage { get; set; }

        const byte FUNCTION_ID = 201;

        public byte[] GetProgram()
        {
            byte[] program = new byte[16];

            program[1] = FUNCTION_ID;
            program[2] = Convert.ToByte(ProbeType);
            program[3] = (byte)(SetPoint >> 8);
            program[4] = (byte)(SetPoint);
            program[5] = HardStart;
            program[6] = MinimumOutputVoltage;


            byte checksum = CalculateTwosComplementChecksum(program);
            program[0] = checksum;

            return program;
        }
        public bool Program()
        {
            string port;

            using (var searcher = new ManagementObjectSearcher("SELECT * FROM Win32_PnPEntity WHERE Caption like '%(COM%'"))
            {
                var portNames = SerialPort.GetPortNames();
                var ports = searcher.Get().Cast<ManagementBaseObject>().ToList().Select(p => p["Caption"].ToString());
                port = portNames.First(port => ports.First(s => s != null && s.Contains(port)).Contains("STMicroelectronics STLink Virtual COM Port"));
            }

            byte[] program = GetProgram();

            SerialPort serialPort = new SerialPort
            {
                PortName = port,
                BaudRate = 19200,
            };
            serialPort.Open();
            serialPort.Write("P");
            for (int i = 0; i < 16; i++)
            {
                serialPort.Write(program, i, 1);
            }
            string result = serialPort.ReadLine();
            serialPort.Close();
            if (result == "PASS")
            {
                return true;
            }
            else
            {
                return false;
            }
        }

        static byte CalculateTwosComplementChecksum(byte[] data)
        {
            int sum = 0;

            // Calculate the sum of all bytes
            foreach (var b in data)
            {
                sum += b;
            }

            // Calculate the 2's complement
            byte checksum = (byte)((~sum + 1) & 0xFF);

            return checksum;
        }
    }
}
