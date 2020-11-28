# I2C-IO-Stepper
Code for Project Struix I2C-IO board as a stepper motor controller

See the Hackaday.io project here:

https://hackaday.io/project/176007-i2c-stepper-contoller

I2C-IO board project here:
https://hackaday.io/project/174984-i2c-io


----------
## I2C Code Interface ##
The device appears on the I2C bus at the default address of <0x7d>.  This address is selectable at compile time in the #defines at the top of the I2C_Stepper code.

Communication is set up as follows:

I2C Write:

	<St><W+7-bit I2C ADDR><A><Register><A><Register Data><A> ... [<Register Data><A>]<Sp>

A write cycle consists of addressing the device, followed by the data register to write, then the data for that register.  The internal register index is updated between each data byte and multiple registers can be written in the same cycle.  In the event there is more data written than there are registers (register overrun), the additional data are simply ignored.

I2C Read:

	<St><R+7-bit I2C ADDR><A><Status Register Read><A><Sp>

A read cycle consists of addressing the device and requesting 1 byte of data.  Only the status register is returned.
----------
There are 5 registers.

	<------------ 8 bits ------------>
	.msb                          lsb.
    *********************************
	* X | X | X | X | X |bsy|on |dir*  R0 --> Status Register
	*********************************
	*         STEPS 0-255           *  R1 --> Steps Register
	*********************************
	*        DELAY (ms) LSB         *  R2 --> Delay between steps (LSB)
	*********************************
	*        DELAY (ms) MSB         *  R3 --> Delay between steps (MSB)
	*********************************
	*              AUX              *  R4 --> Aux Data (future capability)
	*********************************
                   
## Status Register ##
The status register has 3-bits for control:

- dir:(0x01 mask) Controls the direction the stepper motor will take (CW/CCW)
- on:(0x02 mask) Controls whether the stepper motor coils are energized or not
- bsy:(0x04 mask) Status bit to indicate if the controller is actively stepping or not.  

Only the direction and on-bits are user settable.  The dir-bit may be altered mid-cycle.  The on-bit may be disabled at any time, allowing for emergency motor stopping.

## Steps Register ##
The steps register controls how many steps the motor will take once the on-bit is set to 1. The on-bit may be set to 0 at any time, allowing for emergency motor stopping.  The bsy-bit is set whenever the controller is in the middle of a stepping cycle.  The bsy-bit will continue to be set until the cycle completes, regardless of the on-bit status.

## Delay Registers ##
The delay registers set the motor speed and control the amount of time (in milliseconds) between steps.  This delay is a 16-bit value and must be sent in two parts - Least Significant Byte and Most Significant Byte.  The delay may be updated mid-cycle, although the motor behavior will be unknown.

## Auxilluary Register ##
This register is used for extending functionality and future expansion. 


----------
