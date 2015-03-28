# ArduinoHeatedSeatController
# Heated Seat Switching (powered by Arduino Nano)
  By: [Luis E Alvarado](admin@avnet.ws)
  
  [GitHub Repo](https://github.com/avluis/ArduinoHeatedSeatController) | 
  [Hardware Repo](https://github.com/avluis/ArduinoHeatedSeatController-Hardware)

# Purpose:
	To drive a ComfortHeat "Automotive Carbon Fiber Seat Heaters"
  	by Rostra with the stock control panel of a vehicle, in my case, a 2011 Suzuki Kizashi
  [Rostra Kit: 250-1872 - Universal Kit, Double thumb-dial Switch](http://www.rostra.com/manuals/250-1870_Form5261.pdf)
  
# Goals:
    Read input for 2 buttons (driver and passenger)			[DONE]
	Count the number of button presses						[DONE]
	Distinguish button presses (which button was pressed)	[DONE]
	Activate On-Board LED while in ON State					[DONE]
	Trigger output respective to number of button presses	[DONE]
	Slim it all down - optimization phase					[DONE]
	Check for code blocking functions (no delays)			[DONE]
	Add ON signal trigger (pin 10)							[DONE]
	Fix & Optimize HeatLevel								[DONE]
	Set Heat to MID (when on HIGH) after X time				[TESTING]
	Optimize QueryButtonState								[IMPLEMENTING]
	Implement press+hold in QueryButtonState				[IMPLEMENTING]
	Save heat level when button is held						[RESEARCHING]
	Set saved heat level on startup							[RESEARCHING]
