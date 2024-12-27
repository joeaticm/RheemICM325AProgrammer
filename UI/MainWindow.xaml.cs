using Microsoft.Win32;
using Newtonsoft.Json;
using System.Configuration;
using System.Diagnostics;
using System.IO;
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
    public enum MainWindowState
    {
        IDLE,
        PROGRAM_SELECTED,
        PROGRAM_STARTED,
        PROGRAM_FLASHING,
    }

    /// <summary>
    /// Interaction logic for MainWindow.xaml
    /// </summary>
    public partial class MainWindow : Window
    {
        ICM325A? icm;
        MainWindowState state = MainWindowState.IDLE;

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
        const string messageAlreadyProgrammed = "ALREADY PROGRAMMED";


        Configuration configuration = new Configuration
        {
            OutputDirectory = ".",
            Models = [
                    new ICM325A {
                        Model = "YB180",
                        ProbeType = false,
                        SetPoint = 128,
                        HardStart = 50,
                        MinimumOutputVoltage = 17,
                    },
                    new ICM325A {
                        Model = "YB240",
                        ProbeType = false,
                        SetPoint = 123,
                        HardStart = 50,
                        MinimumOutputVoltage = 17,
                    },
                    new ICM325A {
                        Model = "YB300",
                        ProbeType = false,
                        SetPoint = 115,
                        HardStart = 50,
                        MinimumOutputVoltage = 17,
                    }
                ]
        };

        public MainWindow()
        {
            InitializeComponent();
            RemoveICM();
            Status.Text = messageWaiting;
        }

        private void RemoveICM()
        {
            icm = null;
            Program.Visibility = Visibility.Hidden;
            ProgramStatus.Visibility = Visibility.Hidden;

            Title.Text = "";
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

            Title.Text = icm.Model;

            if (icm.ProbeType)
            {
                ProbeType.Text = "Pressure";
                SetPoint.Text = $"{icm.SetPoint} psi";
            }
            else
            {
                ProbeType.Text = "Temperature";
                SetPoint.Text = $"{icm.SetPoint} F";
            }

            HardStart.Text = $"{icm.HardStart / 10.0:0.0} s";
            MinimumOutputVoltage.Text = $"{icm.MinimumOutputVoltage} %";
        }

        private void SetProgram(string model)
        {
            foreach (ICM325A icm in configuration.Models)
            {
                if (model == icm.Model)
                {
                    AddICM(icm);
                }
            }
            state = MainWindowState.PROGRAM_SELECTED;
        }

        private async void WriteProgram(string barcode)
        {
            bool exists = File.Exists(configuration.OutputDirectory + $"\\{barcode}");

            if (exists && configuration.CheckBarcode)
            {
                state = MainWindowState.PROGRAM_SELECTED;
                ProgramStatus.Text = messageAlreadyProgrammed;
                ProgramStatus.Foreground = Brushes.Red;
                return;
            }

            state = MainWindowState.PROGRAM_FLASHING;
            string result = await Task.Run(icm.Program);
            if (result == "PASS")
            {
                ProgramStatus.Text = result;
                ProgramStatus.Foreground = Brushes.Green;

                string probeType = icm.ProbeType ? "Temperature" : "Pressure";

                File.WriteAllText(
                    configuration.OutputDirectory + $"\\{barcode}",
                    $"{icm.Model} ProbeType: {probeType} SetPoint: {icm.SetPoint} HardStart: {icm.HardStart} MinimumOutputVoltage: {icm.MinimumOutputVoltage}");
            }
            else
            {
                ProgramStatus.Text = result;
                ProgramStatus.Foreground = Brushes.Red;
            }
            state = MainWindowState.PROGRAM_SELECTED;
        }

        Configuration? ParseConfiguration(string filePath)
        {
            try
            {
                string jsonData = File.ReadAllText(filePath);

                var jsonObject = JsonConvert.DeserializeObject<Dictionary<string, object>>(jsonData);
                if (jsonObject == null || !jsonObject.ContainsKey("OutputDirectory") || !jsonObject.ContainsKey("Models"))
                {
                    Status.Text = "The configuration file is missing a required key.";
                    return null;
                }

                Configuration config = new Configuration();
                config.OutputDirectory = jsonObject["OutputDirectory"].ToString();

                if (jsonObject.ContainsKey("CheckBarcode") && jsonObject["CheckBarcode"] is bool)
                {
                    config.CheckBarcode = (bool)jsonObject["CheckBarcode"];
                }

                List<ICM325A> models = new List<ICM325A>();
                foreach (var modelObj in (Newtonsoft.Json.Linq.JArray)jsonObject["Models"])
                {
                    var modelDict = modelObj.ToObject<Dictionary<string, object>>();

                    if (!(
                        modelDict != null &&
                        modelDict.ContainsKey("Model") &&
                        modelDict.ContainsKey("ProbeType") &&
                        modelDict.ContainsKey("SetPoint") &&
                        modelDict.ContainsKey("HardStart") &&
                        modelDict.ContainsKey("MinimumOutputVoltage")
                        ))
                    {
                        Status.Text = "One or more of the models is missing a required parameter.";
                        return null;
                    }

                    ICM325A model = new ICM325A
                    {
                        Model = modelDict["Model"].ToString(),
                        ProbeType = ParseProbeType(modelDict["ProbeType"].ToString()),
                        SetPoint = Convert.ToUInt16(modelDict["SetPoint"]),
                        HardStart = Convert.ToByte(modelDict["HardStart"]),
                        MinimumOutputVoltage = Convert.ToByte(modelDict["MinimumOutputVoltage"])
                    };

                    if (!model.IsValidModel())
                    {
                        Status.Text = "One or more of the parameters is outside the supported range.";
                        return null;
                    }

                    models.Add(model);
                }

                config.Models = models;
                Status.Text = "Successfully loaded configuration. Scan a barcode to select configuration.";
                return config;
            }
            catch (IOException)
            {
                Status.Text = "Failed to opened the configuration file.";
                return null;

            }
            catch (JsonException)
            {
                Status.Text = "The configuration file was not a valid JSON document.";
                return null;
            }
            catch (OverflowException)

            {
                Status.Text = "One or more of the parameters is outside the supported range";
                return null;
            }
            catch (Exception)
            {
                Status.Text = "Error occurred while loading configuration.";
                return null;
            }
        }

        static bool ParseProbeType(string probeType)
        {
            string lowerType = probeType.ToLower();
            return lowerType == "pressure" || lowerType == "p" || lowerType == "press";
        }

        private void ProgramButton_Click(object sender, RoutedEventArgs e)
        {
            if (state != MainWindowState.PROGRAM_SELECTED)
            {
                return;
            }
            if (icm != null)
            {
                state = MainWindowState.PROGRAM_STARTED;
                ProgramStatus.Text = messageScanIcm;
                ProgramStatus.Foreground = Brushes.Black;
            }
        }

        private void ConfigureButton_Click(object sender, RoutedEventArgs e)
        {
            if (state != MainWindowState.IDLE && state != MainWindowState.PROGRAM_SELECTED)
            {
                return;
            }

            OpenFileDialog dialog = new OpenFileDialog();
            dialog.Filter = "Configuration File (*.json)|*.json";
            dialog.Title = "Select configuration file...";

            var success = dialog.ShowDialog();

            if (success == true)
            {
                Configuration? newConfiguration = ParseConfiguration(dialog.FileName);
                if (newConfiguration != null)
                {
                    state = MainWindowState.IDLE;
                    RemoveICM();
                    configuration = newConfiguration;
                }
            }
        }

        private void Window_PreviewKeyDown(object sender, KeyEventArgs e)
        {
            e.Handled = true;
            if (Key.Return == e.Key)
            {
                if (icmBarcode.Match(barcode).Success)
                {
                    if (state == MainWindowState.PROGRAM_STARTED)
                    {
                        ProgramStatus.Text = messageRunning;
                        WriteProgram(barcode);
                    }
                }
                if (rheemBarcode.Match(barcode).Success)
                {
                    if (state == MainWindowState.IDLE || state == MainWindowState.PROGRAM_SELECTED)
                    {
                        string program = rheemBarcode.Match(barcode).Groups[2].Value;
                        SetProgram(program);
                    }
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