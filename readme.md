# QtCampp
## inspired by qtcam

I have a simple problem: webcamera is inserted into telescope and I have about 30s to make pictures before planets are gone out of view. Also I need steady control over settings, like exposure. In field USB cable may get disconnected, so if I insert it back I want camera be ready to picture without mouse clicks (well, hate trackpads). No one of 4 programs I tried worked in this way, most annoying was cable disconnection and setup camera again.

### This program is going to solve issues listed above, keys are:

* it keeps all camera settings saved on computer, if same named camera selected it applies all saved
settings automatically;
* if camera is reconnected to the same usb slot it is able to pick it and re-apply settings and continue;
* last used camera will be opened on start if present.

So main idea behind is - "less clicks!".

Program is based on GPL sources which I used as examples (because it is 1st time I use V4L), however, sources were 7 years old till today and had great errors (which I bet lead all that 4 programs to crashes I saw in fact). So I rewrote pieces I needed exactly for this task from scratch.

__All together is MIT licensed.__

_Requires modern (as for 2016) C++ to be compiled._

_P.S. I have only 1 camera for tests (which have about 8 control fields), so if anything is missing - feel free to add._
