using System.Diagnostics;
using System.IO.Ports;
using System.Text;
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

        const string messageWaiting = "Scan a barcode to select configuration.";
        const string messageReady = "Press \"Program\" to write configuration to ICM325A.";

        const string messageIdle = "IDLE";
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

        private async void Button_Click(object sender, RoutedEventArgs e)
        {
            if (programmer_running)
            {
                return;
            }
            programmer_running = true;
            if (icm != null)
            {

                ProgramStatus.Text = messageRunning;
                bool result = await Task.Run(icm.Program);
                if (result)
                {
                    ProgramStatus.Text = messagePass;
                }
                else
                {
                    ProgramStatus.Text = messageFail;
                }
                programmer_running = false;
            }
        }

        private void Window_KeyDown(object sender, KeyEventArgs e)
        {
            if (e.Key == Key.A)
            {
                ICM325A icm = new ICM325A();
                icm.ProbeType = true;
                icm.SetPoint = 55;
                icm.HardStart = 30;
                icm.MinimumOutputVoltage = 40;
                AddICM(icm);
            }
            else if (e.Key == Key.B)
            {
                ICM325A icm = new ICM325A();
                icm.ProbeType = false;
                icm.SetPoint = 400;
                icm.HardStart = 40;
                icm.MinimumOutputVoltage = 17;
                AddICM(icm);
            }
        }
    }
}