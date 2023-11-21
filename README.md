# Digital_spool_holder V2

![An image of the spool holder project](https://img.thingiverse.com/cdn-cgi/image/fit=contain,quality=95,width=640/https://cdn.thingiverse.com/assets/a4/cd/1e/ce/07/large_display_8fc11203-5cf1-4a74-8cd6-9984aafc7cdd.jpeg)

A digital spool holder for 3d printers. Based on the original by InterlinkKnight
License: [CC-BY-NC](https://creativecommons.org/licenses/by-nc/4.0/)

* Share & adapt; attribution is required. Non-commercial use only.

Digital Spool Holder (with Scale) by InterlinkKnight on Thingiverse: <https://www.thingiverse.com/thing:5677322>

I have called this V2 in respect only of the software. The hardware remains identical to the original. I have added my remix of the spool holder base that works with the twist-fit Creality K1 and K1 Max, but beyond that, all the electronics remain unchanged. On the software, however, I wanted to add the capability to estimate the expected spool weight using the total weight of the new spool and the nominal/expected filament weight.

For example, I have a spool of Creality PLA that is nominally carrying 1.1kg of PLA and weighs 1.2kg. We can determine that the spool should weigh 100g.

When I examined the existing code, I found a lot of repetition that may have been intended for performance purposes but which, for my needs, was going to make maintenance a lot harder. Thus, we have V2, which is extensively refactored and hopefully easier to maintain and extend. There's quite a lot more that could be done; breaking out the menu handling to be more data-driven, for example, but as this is intended to run on a nano, where data space is severely constrained, we need to be mindful of this and so, for now, I've not gone that far.

A massive thank you to InterlinkKnight for the brilliant project and all the work on the original code, which is a very usable and well-thought-out interface for the task.

Neil @ Zyxt

## More documentation

### Tutorials and explanations

By far, the best documentation is the [excellent tutorial by InterlinkKnight](https://www.instructables.com/Digital-Spool-Holder-with-Scale/). He also has [a great video](https://www.youtube.com/watch?v=WO-hR7okl3k&ab_channel=InterlinkKnight) demonstrating the reasons behind his design choices.

### The code

The code is intended to be largely standalone; it is fully configurable at runtime and should not need modification for most purposes. If you are reading this, the chances are you have already found the GitHub repo.

### Circuit boards

As the original creator notes at various points, his PCB designs are available on PCBWay, or you may use the Gerber files he provides to send to a fabricator of your choice (or make your own).

The easiest way is to simply find the [digital spool holder project](https://www.pcbway.com/project/shareproject/Digital_Spool_Holder_with_Scale_3b6cab54.html) on PCBWay's shared project platform and "add to basket." That is what I did, and five immaculate PCBs turned up a few weeks later, which gave me time to gather the rest of the items. By using InterlinkKnight's project above, you will also support him (in a very tiny way) as PCBWay donates 10% of the cost to the Author.

### General hardware and electronics

All the components that you will need are listed as part of the Instructables site. I purchased OLED Displays from eBay. I bought the HX711 strain gauges on Amazon along with any other items I didn't already have.

### Printables

The main printables are found on [InterlinkKnight's](https://www.thingiverse.com/thing:5677322)Thingiverse[listing](https://www.thingiverse.com/thing:5677322). You will also find a number of remixes listed there, including my own remix of the spool holder base for the K1/K1 Max.

## Notable changes

* Changed the EEPROM to read/write the entire struct in one go using the `get()`/`put()` methods.
* Refactored the button handling logic to a single stand-alone function that takes a button struct as input, removing the duplication previously required.
* Extracted the debounce logic at the same time to provide a separation mostly for testing.
* Rewrote the title editor to remove the extensive static string storage and replace it with a dynamically generated menu.
* Refactored the button handling to occur once at the start of the loop.
* Removed the `[next]` button as it has no use in the current editor which is always appending.
* Updated to u8g2 library as this appears to be supported still.
* u8g2 required a few changes around the string handling for PROGMEM/non-PROGMEM strings.
* Reworked the profile sequence handling (this needs to be appropriately tested and may remain buggy).
* I am using an 1106 display; a compatible 1306 driver is commented out in the code. The 1306 has a different display offset that causes clipping and wrapping when used on the 1106. If you notice an offset issue, you might want to swap the driver and re-test.
* No longer store the title length or active/disabled in profile.
* NEW FEATURE: Added Tare using full spool to the Tare menus. This allows the profile to tare when all you have is a new spool and no similar empty spools.
