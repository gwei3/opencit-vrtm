using System;
using System.Collections.Generic;
using System.ComponentModel;
using System.Data;
using System.Diagnostics;
using System.Linq;
using System.ServiceProcess;
using System.Text;
using System.Threading.Tasks;
using System.Timers;

namespace vRTM
{
    public partial class vrtmservice : ServiceBase
    {
        private Timer timer = null;

        public vrtmservice()
        {
            InitializeComponent();
        }

        private void timer_Tick(object sender, ElapsedEventArgs e)
        { 
            ServiceController sc = new ServiceController("vRTM");
            switch(sc.Status)
            {
                case ServiceControllerStatus.Running:
                    return;
                default:
                    Library.WriteErrorLog("Starting vRTM service");
                    this.start_vrtm();
                    return;
            }
        }

        private void start_vrtm()
        {
            string vrtm_dir = Environment.GetEnvironmentVariable("VRTM_HOME");
            if (vrtm_dir == null)
            {
                Library.WriteErrorLog("VRTM_HOME environment variable not found");
                Library.WriteErrorLog("Service could not be started");
                return;
            }
            string vrtmcmdstr = vrtm_dir + "/bin/vrtm.cmd";
            Library.WriteErrorLog(vrtmcmdstr);

            Process myProcess = new Process();
            try
            {
                myProcess.StartInfo.UseShellExecute = false;
                myProcess.StartInfo.FileName = vrtmcmdstr;
                myProcess.StartInfo.Arguments = "start";
                myProcess.StartInfo.CreateNoWindow = true;
                myProcess.Start();
            }
            catch (Exception ex)
            {
                Library.WriteErrorLog(ex.Message);
                this.stop_vrtm();
            }
        }

        private void stop_vrtm()
        {
            string vrtm_dir = Environment.GetEnvironmentVariable("VRTM_HOME");
            if (vrtm_dir == null)
            {
                Library.WriteErrorLog("VRTM_HOME environment variable not found");
                Library.WriteErrorLog("Service could not be stopped");
                return;
            }
            string vrtmcmdstr = vrtm_dir + "/bin/vrtm.cmd";
            Library.WriteErrorLog(vrtmcmdstr);

            Process myProcess = new Process();
            try
            {
                myProcess.StartInfo.UseShellExecute = false;
                myProcess.StartInfo.FileName = vrtmcmdstr;
                myProcess.StartInfo.Arguments = "stop";
                myProcess.StartInfo.CreateNoWindow = true;
                myProcess.Start();
            }
            catch (Exception ex)
            {
                Library.WriteErrorLog(ex.Message);
                this.stop_vrtm();
            }
        }

        protected override void OnStart(string[] args)
        {
            timer = new Timer();
            this.timer.Interval = 60000;
            this.timer.Elapsed += new System.Timers.ElapsedEventHandler(this.timer_Tick);

            this.start_vrtm();
            timer.Enabled = true;
            Library.WriteErrorLog("vRTM started");
        }

        protected override void OnStop()
        {
            this.stop_vrtm();
            timer.Enabled = false;
            Library.WriteErrorLog("vRTM stopped");
        }
    }
}
