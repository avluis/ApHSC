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
	Select timer duration out of predetermined options				[DONE]
	Swap buttons for press+hold event								[DONE]
	Blink current selected Heat level when indicating				[DONE]
	Don't turn on main power (status pin) when indicating			[DONE]
	Modify auto start heat level saving to always save				[DONE]
	Disable auto start when heat is OFF for both sides				[DONE]
	Add a proper (GNU v2 Only) license								[DONE]
	Rewrite Blink function for non-blocking code					[DONE]
	HeatTimer will run on whichever side is not OFF					[DONE]
	Disable Auto Startup when both sides are OFF					[DONE]
	Change the onBoardLed to indicate HeartBeat						[DONE]
	Cancel HeatTimer if Heat Level is changed (either side)			[DONE]
	Add more debug options (separate from prior debug option)		[DONE]
	Separated .ino into multiple files [VS Micro compatible]		[DONE]
