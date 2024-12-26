using System.Diagnostics;
using System.IO.Ports;
using System.Text;
using System.Text.RegularExpressions;
using System.Windows;
using System.Windows.Controls;
using System.Windows.Data;
using System.Windows.Documents;
using System.Windows.Input;
using System.Windows.Media;
using System.Windows.Media.Imaging;
using System.Windows.Navigation;
using System.Windows.Shapes;

namespace RheemICM325AProgrammer
{
    /// <summary>
    /// Interaction logic for MainWindow.xaml
    /// </summary>
    public partial class MainWindow : Window
    {
        ICM325A? icm;
        bool programmer_running = false;

        string barcode = "";
        Regex icmBarcode = new Regex(@"^W?([0-9]{6})S([0-9]{10})10D([0-9]{4})$");
        Regex rheemBarcode = new Regex(@"^W?([A-Z0-9]{4})(YB[0-9]{3})([A-Z0-9]{12})$");

        const string messageWaiting = "Scan a barcode to select configuration.";
        const string messageReady = "Press \"Program\" to write configuration to ICM325A.";

        const string messageIdle = "IDLE";
        const string messageScanIcm = "SCAN ICM";
        const string messageRunning = "RUNNING";
        const string messagePass = "PASS";
        const string messageFail = "FAIL";

        public MainWindow()
        {
            InitializeComponent();
            RemoveICM();
        }

        private void RemoveICM()
        {
            icm = null;
            Status.Text = messageWaiting;
            Program.Visibility = Visibility.Hidden;
            ProgramStatus.Visibility = Visibility.Hidden;

            ProbeType.Text = "";
            SetPoint.Text = "";
            HardStart.Text = "";
            MinimumOutputVoltage.Text = "";
        }

        private void AddICM(ICM325A icm)
        {
            this.icm = icm;
            Status.Text = messageReady;
            Program.Visibility = Visibility.Visible;
            ProgramStatus.Text = messageIdle;
            ProgramStatus.Foreground = Brushes.Black;
            ProgramStatus.Visibility = Visibility.Visible;

            if (icm.ProbeType)
            {
                ProbeType.Text = "Pressure";
            }
            else
            {
                ProbeType.Text = "Temperature";
            }

            SetPoint.Text = $"{icm.SetPoint} psi";
            HardStart.Text = $"{icm.HardStart / 10.0:0.0} s";
            MinimumOutputVoltage.Text = $"{icm.MinimumOutputVoltage} %";
        }

        private void SetProgram(string code)
        {
            if (code == "YB300")
            {
                ICM325A icm = new ICM325A();
                icm.ProbeType = false;
                icm.SetPoint = 400;
                icm.HardStart = 40;
                icm.MinimumOutputVoltage = 17;
                AddICM(icm);
            }
        }

        private async void WriteProgram()
        {
            bool result = await Task.Run(icm.Program);
            if (result)
            {
                ProgramStatus.Text = messagePass;
                ProgramStatus.Foreground = Brushes.Green;
            }
            else
            {
                ProgramStatus.Text = messageFail;
                ProgramStatus.Foreground = Brushes.Red;
            }
            programmer_running = false;

        }

        private void Button_Click(object sender, RoutedEventArgs e)
        {
            if (programmer_running)
            {
                return;
            }
            programmer_running = true;
            if (icm != null)
            {

                ProgramStatus.Text = messageScanIcm;
                ProgramStatus.Foreground = Brushes.Black;
            }
        }

        private void Window_PreviewKeyDown(object sender, KeyEventArgs e)
        {
            if (Key.Return == e.Key)
            {
                if (icmBarcode.Match(barcode).Success)
                {
                    if (programmer_running)
                    {
                        WriteProgram();
                    }
                }
                if (rheemBarcode.Match(barcode).Success)
                {
                    string program = rheemBarcode.Match(barcode).Groups[2].Value;
                    SetProgram(program);
                }

                barcode = "";
            }
            else
            {
                if (e.Key >= Key.A && e.Key <= Key.Z)
                {
                    barcode += e.Key;
                }
                if (e.Key >= Key.D0 && e.Key <= Key.D9)
                {
                    barcode += (int)e.Key - 34;
                }
            }
        }
    }
}