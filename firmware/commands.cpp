#include "commands.h"
#include "command.h"
#include "calibration.h"
#include "stepper_controller.h"
#include <stdlib.h>
#include "nonvolatile.h"
#include "Reset.h"


#define COMMANDS_PROMPT (":>")
sCmdUart UsbUart;

CMD_STR(help,"Displays this message");
CMD_STR(getcal,"Prints the calibration table");
CMD_STR(calibrate,"Calbirates the encoder, should be done with motor disconnected from machine");
CMD_STR(testcal,"tests the calibaration of the encoder");
CMD_STR(microstep,"gets/sets the microstep size, example 'microstep 16'");
CMD_STR(step, "Steps motor one step, optionally direction can be set is 'step 1' for reverse");
CMD_STR(feedback, "enable or disable feedback controller, 'feedback 0' - disables, 'feedback 1' - enables");
CMD_STR(readpos, "reads the current angle as 16bit number, applies calibration if valid");
CMD_STR(encoderdiag, "Prints encoder diagnositc")
CMD_STR(pid, "with no arguments prints PID parameters, with arguments sets PID 'PID Kp Ki Kd Threshold' "
		"Where Kp,Ki,Kd,Threshold are int32_t numbers")
CMD_STR(testringing ,"Steps motor at various currents and measures encoder")
CMD_STR(microsteperror ,"test error on microstepping")
CMD_STR(params, "with no arguments read parameters, will set 'params currentMa holdCurrentMa errorLimit'")
CMD_STR(boot, "Enters the bootloader")
CMD_STR(move, "moves encoder x degrees at y ma 'move x y'")
CMD_STR(test, "test")
//List of supported commands
sCommand Cmds[] =
{
		COMMAND(help),
		COMMAND(calibrate),
		COMMAND(getcal),
		COMMAND(testcal),
		COMMAND(microstep),
		COMMAND(step),
		COMMAND(feedback),
		COMMAND(readpos),
		COMMAND(encoderdiag),
		COMMAND(pid),
		COMMAND(testringing),
		COMMAND(microsteperror),
		COMMAND(params),
		COMMAND(boot),
		COMMAND(move),
		COMMAND(test),
		{"",0,""}, //End of list signal
};

static int test_cmd(sCmdUart *ptrUart,int argc, char * argv[])
{
	int32_t x=0;;
	int32_t i;

	for (i=0; i<100; i++)
	{
		x=stepperCtrl.getCurrentLocation()+300;
		LOG("x is %d, %d",x,stepperCtrl.getCurrentLocation());
		stepperCtrl.moveToAngle(x,2000);
		delay(100);
		stepperCtrl.process();
	}
	return 0;
}

static int move_cmd(sCmdUart *ptrUart,int argc, char * argv[])
{
	int32_t x,ma;
	CommandPrintf(ptrUart, "Move %d",argc);
	if (2 == argc)
	{
		float f;
		f=atof(argv[0]);
		//		if (f>1.8)
		//			f=1.8;
		//		if (f<-1.8)
		//			f=-1.8;
		x=ANGLE_FROM_DEGREES(f);
		LOG("moving %d", x);
		ma=atoi(argv[1]);

		stepperCtrl.moveToAngle(x,ma);
	}
	return 0;
}

static int boot_cmd(sCmdUart *ptrUart,int argc, char * argv[])
{
	initiateReset(250);
}

static int microsteperror_cmd(sCmdUart *ptrUart,int argc, char * argv[])
{
	int i,n,j;
	bool feedback=stepperCtrl.getFeedback();
	n=200*stepperCtrl.getMicroSteps();

	CommandPrintf(ptrUart, "Function needs fixed");
	return 0;
	stepperCtrl.feedback(false);
	for (j=0; j<2; j++)
	{
		for (i=0; i<n; i++)
		{
			int32_t e;
			stepperCtrl.requestStep(1,1);
			//stepperCtrl.step(1, 2000);
			stepperCtrl.process();

			//average 1readings
			int32_t sum=0,ii;
			for (ii=0; ii<1; ii++)
			{
				sum+=stepperCtrl.measureError();
				stepperCtrl.process();
			}
			e=sum/ii;
			CommandPrintf(ptrUart,"%d %d\n\r",i,e);
		}
	}
	stepperCtrl.feedback(feedback); //restore feedback
	return 0;
}
static int testringing_cmd(sCmdUart *ptrUart,int argc, char * argv[])
{
	stepperCtrl.testRinging();
	return 0;
}

static int params_cmd(sCmdUart *ptrUart,int argc, char * argv[])
{
	if (0 == argc)
	{
		CommandPrintf(ptrUart,"currentMa %d\n\r",NVM->SystemParams.currentMa);
		CommandPrintf(ptrUart,"currentHoldMa %d\n\r",NVM->SystemParams.currentHoldMa);
		CommandPrintf(ptrUart,"errorLimit %d\n\r",NVM->SystemParams.errorLimit);

	}
	if (3 == argc)
	{
		int32_t currentMa,currentHoldMa,errorLimit;
		currentMa=atol(argv[0]);
		currentHoldMa=atol(argv[1]);
		errorLimit=atol(argv[2]);
		nvmWriteSystemParms(currentMa,currentHoldMa,errorLimit);
		CommandPrintf(ptrUart,"currentMa %d\n\r",NVM->SystemParams.currentMa);
		CommandPrintf(ptrUart,"currentHoldMa %d\n\r",NVM->SystemParams.currentHoldMa);
		CommandPrintf(ptrUart,"errorLimit %d\n\r",NVM->SystemParams.errorLimit);

	}
	return 0;
}

static int pid_cmd(sCmdUart *ptrUart,int argc, char * argv[])
{
	if (0 == argc)
	{
		CommandPrintf(ptrUart,"Kp %d\n\r",NVM->PIDparams.Kp);
		CommandPrintf(ptrUart,"Ki %d\n\r",NVM->PIDparams.Ki);
		CommandPrintf(ptrUart,"Kd %d\n\r",NVM->PIDparams.Kd);
		CommandPrintf(ptrUart,"Threshold %d\n\r",NVM->PIDparams.Threshold);
	}
	if (4 == argc)
	{
		int32_t Kp,Ki,Kd,Threshold;
		Kp=atol(argv[0]);
		Ki=atol(argv[1]);
		Kd=atol(argv[2]);
		Threshold=atol(argv[3]);
		nvmWritePID(Kp,Ki,Kd,Threshold);
		CommandPrintf(ptrUart,"Kp %d\n\r",NVM->PIDparams.Kp);
		CommandPrintf(ptrUart,"Ki %d\n\r",NVM->PIDparams.Ki);
		CommandPrintf(ptrUart,"Kd %d\n\r",NVM->PIDparams.Kd);
		CommandPrintf(ptrUart,"Threshold %d\n\r",NVM->PIDparams.Threshold);
	}
	return 0;
}
static int encoderdiag_cmd(sCmdUart *ptrUart,int argc, char * argv[])
{
	char str[512];
	stepperCtrl.encoderDiagnostics(str);
	CommandPrintf(ptrUart,"%s",str);
	return 0;
}

static int readpos_cmd(sCmdUart *ptrUart,int argc, char * argv[])
{
	uint16_t pos,a;
	int32_t x;

	pos=stepperCtrl.measureMeanEncoder();
	CommandPrintf(ptrUart,"encoder %d",pos);
	return 0;
}
static int feedback_cmd(sCmdUart *ptrUart,int argc, char * argv[])
{
	if (0 == argc)
	{
		CommandPrintf(ptrUart,"must pass argument, 'feedback 0' - disables, 'feedback 1' - enables");
		return 1;
	}
	stepperCtrl.feedback(atoi(argv[0]));
	return 0;
}

static int step_cmd(sCmdUart *ptrUart,int argc, char * argv[])
{
	if (argc == 0 )
	{
		stepperCtrl.move(0, 1);
		//stepperCtrl.step(STEPPER_FORWARD);
	}else
	{
		int d, steps=1;
		d=atoi(argv[0]);
		if (argc >1)
		{
			steps=atoi(argv[1]);
		}
		if (1 == d)
		{
			stepperCtrl.move(1, steps);
		} else
		{
			stepperCtrl.move(0, steps);
		}
	}
	return 0;
}

static int microstep_cmd(sCmdUart *ptrUart,int argc, char * argv[])
{
	bool ret;

	if (argc != 1)
	{
		CommandPrintf(ptrUart,"%d microsteps\n\r", stepperCtrl.getMicroSteps());
		return 0;
	}
	ret=stepperCtrl.changeMicrostep(atoi(argv[0]));
	if (ret != true)
	{
		CommandPrintf(ptrUart,"Could not set microstep\n\r");
	}
	return 0;
}


// print out the help strings for the commands
static int help_cmd(sCmdUart *ptrUart,int argc, char * argv[])
{
	sCommand cmd_list;
	int i;

	//now let's parse the command
	i=0;
	memcpy(&cmd_list, &Cmds[i], sizeof(sCommand));
	while(cmd_list.function!=0)
	{

		CommandPrintf(ptrUart,(cmd_list.name));
		CommandPrintf(ptrUart,(" - "));
		CommandPrintf(ptrUart,(cmd_list.help));
		CommandPrintf(ptrUart,("\n\r"));
		i=i+1;
		memcpy(&cmd_list, &Cmds[i], sizeof(sCommand));
	}
	return 0;
}



static int getcal_cmd(sCmdUart *ptrUart,int argc, char * argv[])
{
	stepperCtrl.calTable.printCalTable();
	return 0;
}

static int calibrate_cmd(sCmdUart *ptrUart,int argc, char * argv[])
{
	stepperCtrl.calibrateEncoder();
	CommandPrintf(ptrUart,"Calibration DONE!\n\r");
	return 0;
}

static int testcal_cmd(sCmdUart *ptrUart,int argc, char * argv[])
{
	Angle a;
	int32_t x;

	a=stepperCtrl.maxCalibrationError();
	x=(uint16_t)a*(int32_t)360000L/(int32_t)ANGLE_MAX;

	CommandPrintf(ptrUart,"Max error is %d.%03d degrees\n\r", x/1000,x%1000);
	return 0;
}




uint8_t kbhit(void)
{
	return Serial.available();
}
uint8_t getChar(void)
{
	return Serial.read();
}
uint8_t putch(char data)
{
	return Serial.write((uint8_t)data);
}


void commandsInit(void)
{
	CommandInit(&UsbUart, kbhit, getChar, putch ,NULL); //set up the UART structure
	Serial.print("\n\rPower Up\n\r");
	Serial.print(COMMANDS_PROMPT);
}

int commandsProcess(void)
{
	return CommandProcess(&UsbUart,Cmds,' ',COMMANDS_PROMPT);
}
