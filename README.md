# Arduino Heated Seat Controller
# Aftermarket Heated Seat Switching (powered by Arduino Nano)
  By: [Luis E Alvarado](mailto:admin@avnet.ws)
  
  [Software](https://github.com/avluis/ArduinoHeatedSeatController) | 
  [Hardware](https://github.com/avluis/ArduinoHeatedSeatController-Hardware)

# Purpose:
	To drive a ComfortHeat "Automotive Carbon Fiber Seat Heaters"
  	by Rostra with the stock control panel of a vehicle, in my case, a 2011 Suzuki Kizashi
   [Rostra Kit: 250-1872 - Universal Kit, Double thumb-dial Switch](http://www.rostra.com/manuals/250-1870_Form5261.pdf)
  
# Goals (in order of implementation):
    Read input for 2 buttons (driver and passenger)					[DONE]
	Count the number of button presses								[DONE]
	Distinguish button presses (which button was pressed)			[DONE]
	Activate On-Board LED while in ON State							[DONE]
	Trigger output respective to number of button presses			[DONE]
	Slim it all down - optimization phase							[DONE]
	Check for code blocking functions (no delays)					[DONE]
	Add ON signal trigger (pin 10)									[DONE]
	Fix & Optimize HeatLevel										[DONE]
	Set Heat to MID (when on HIGH) after X time						[DONE]
	Optimize QueryButtonState										[DONE]
	Implement press+hold in QueryButtonState						[DONE]
	Enable/Disable timer when (driver) button is held				[DONE]
	Implement Auto Startup - (HIGH Heat)							[DONE]
	Enable/Disable Auto Startup when (passenger) button is held		[DONE]
	Implement a status indicator (Blink)							[DONE]
	Implement different blink patters								[DONE]
	Re-write HeatTimer												[DONE]
	Only run timer once per start (or autostart)					[DONE]
	Only run timer if autostart is enabled							[DONE]
	Save autostart Heat level when (passenger) button is held		[DONE]
	Startup on saved heat level										[DONE]
	Only run timer if autostart heat level is set to HIGH			[DONE]
	Select timer duration out of predetermined options				[TESTING]
	Swap buttons for press+hold event								[TESTING]
	Blink current selected Heat level when indicating				[RESEARCHING]
	Remove delay function(s) from Blink method						[RESEARCHING]
