/*
 * Shuangyi Tong <shuangyi.tong@stcatz.ox.ac.uk> created on 05 April 2024
 */

using System;
using System.IO;
using System.Net;
using System.Net.Sockets;
using System.IO.Ports;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading.Tasks;
using System.Threading;
using System.Data.Common;
using System.Collections;


namespace QSTLabSerialControl
{
    public class PainlabProtocolException : Exception
    {
        public PainlabProtocolException()
        {
        }

        public PainlabProtocolException(string message)
            : base(message)
        {
        }

        public PainlabProtocolException(string message, Exception inner)
            : base(message, inner)
        {
    }
    public class QSTLabSerialControl
    {
        SerialPort sp;
        public void Init(int com_port = "COM5") // The COM port for QST device registered on the lab PC
        {
            sp = new SerialPort(com_port, 115200);
            sp.Open();
            sp.ReadTimeout = 5000;

            sp.Write("F"); // silence the useless information , not necessary here if not going to get the temperature post-stimulation
        }

        // use sp.Write to write command directly, otherwise you can use the following helper functions

        public void SetBaseTemperature(float t) // one decimal place accuracy only
        {
            int no_decimal = Math.Round(t * 10);
            if (no_decimal > 450 || no_decimal < 200)
            {
                throw PainlabProtocolException("QST base temperature limits are within 20 - 45 degrees");
            }

            sp.Write("N" + no_decimal.ToString("000"));
        }

        public void SetTargetTemperature(float t)
        {
            int no_decimal = Math.Round(t * 10);
            if (no_decimal > 600 || no_decimal < 0)
            {
                throw PainlabProtocolException("QST target temperature are within 0 - 60 degrees");
            }
            
            // you can change 0 here to arbitrary single surface 1-5 on the probe, 0 is for all. Refer to the manual for more information
            sp.Write("C0" + no_decimal.ToString("000"));
        }

        public void SetDuration(int duration) // in milliseconds
        {
            if (duration > 99999 || duration < 10)
            {
                throw PainlabProtocolException("QST stimulation duration can only between 10 - 99999 milliseconds");
            }

            sp.Write("D0" + duration.ToString("00000"));
        }

        public void StartStimulation()
        {
            sp.Write("L");
        }
    }
}
