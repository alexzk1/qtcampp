# QtCampp
## inspired by qtcam

I have a simple problem: webcamera is inserted into telescope and I have about 30s to make pictures before planets are gone out of view. Also I need steady control over settings, like exposure. In field USB cable may get disconnected, so if I insert it back I want camera be ready to picture without mouse clicks (well, hate trackpads). No one of 4 programs I tried worked in this way, most annoying was cable disconnection and setup camera again.

### This program is going to solve issues listed above, keys are:

* it keeps all camera settings saved on computer, if same named camera selected it applies all saved
settings automatically;
* if camera is reconnected to the same usb slot it is able to pick it and re-apply settings and continue;
* for each camera program keeps 10 separated preset settings. When you switch to "never used yet" it gets copied from previsious active. It allows to make fast switches using keyboard shortcuts;
* there are "night mode" and "fullscreen mode", in conjunction those both allow to use small notebook (like 1024 x 600 screen) to make pictures in the field at the night time over telescope;
* last used camera will be opened on start if present;
* camera settings can be dragged top-down and realigned to access most used without scrolling (like exposure).

### Limitations:

* devices are recognized by human readable names, 2 with same name will be considered the sames, so case when 2 same named connected is not checked (most likelly will work first found);
* implemented only subset of V4L API to access devices, minimum needed to get picture and configure __my__ webcamera. Sound etc. is not implemented intentionally and is not checked. Maybe will work.

So main idea behind is - "less clicks!".

Program is based on GPL sources which I used as examples (because it is 1st time I use V4L), however, sources were 7 years old till today and had great errors (which I bet lead all that 4 programs to crashes I saw in fact). So I rewrote pieces I needed exactly for this task from scratch.

__All together (contained in this repository) is MIT licensed, no matter if file itself mentiones it. Icons/arts may have separated licenses mentioned in "licenses.txt".__

_Requires modern (as for 2016) C++ to be compiled._

_P.S. I have only 1 camera for tests (which have about 8 control fields), so if anything is missing - feel free to add._
