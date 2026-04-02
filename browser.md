# Building the Web (Again)

## The goal of this PS is to build a web (not the real one) browser.

## Of course, several people have tried and failed like Microsoft to build a browser from

## scratch, but you will do it in a week. Good luck.

## The only advantage you do have is that the web we are talking about is a simpler imitation

## of the modern web.

## JSML

## JSML is HTML + CSS rolled into one language.

## This is an example page showing all the 3 tags available.

### {

```
"lua": "/script.lua",
"root": {
"flexv": [
{
"text": {
"content": "Hello, world!",
"fontsize": 14 , // [o] defaults to 14
"color": "#000000"
},
"tag" : 1 //id of the element
"bgcolor": "#fffffff", // [o]
"spacing": 1 // [o] defaults to 1
"onclick" : "functionabcd()"
},
{
"image": {
"url": "/cat.png",
"alttext": "Cat Image" // [o] defaults to empty
string
},
"bgcolor": "#fffffff", // [o]
"spacing": 1 // [o] defaults to 1
},
],
```

## Each element is of the form:

## ElementData can be of the form:

## flexv will place elements vertically respecting the spacing property of each child. Refer

## to flex in the real web standards to learn more. Similarly flexh will place items

## horizontally.

## Scripting

## As you might have noticed there is a Lua file referred at the top of the example page.

## Refer to the LuaBrowser docs to see the other half of the PS.

# Wait, what? How do I render?

## You decide. Pygame, SDL, raylib, vulkan maybe? your choice.

### }

### }

```
"ElementData": [] / {}, // can be flexv flexh text or image
"tag": "tag of element",
"bgcolor" : "background color of the element"
"spacing" : 1 // optional and defaults to 1,
"onclick" : "function to be called on click"
```
```
"flexv" or "flexh": [
// includes other elements like text and image and possibly other
flexes
],
"text": {
"content": "Content of the tag",
"fontsize": 14 , // font size for the text to be rendered
"color": "#000000" // color of the text to be rendered
},
"image": {
"url": "path of the image",
"alttext": "Alternate text in case the image doesn't exist or
load"
}
```

# Requirements

## The end goal will include:

# Judging Criteria

## The first criteria is correctness.

## As the PS progresses, we will release some test JSML pages. These pages should render

## correctly.

## The second criteria is speed.

## We will be writing benchmark code that renders lots of elements to screen. Your job is to

## make it as fast as possible.

## Submission

## An app that runs on linux.

## We expect to ideally get a single executable. However since some languages do not

## compile to executables, you can send a folder with a run.sh script.

## Do not worry, we will try to communicate with you if your browser does not open.

## an app that works on atleast linux, cross-platform welcome but we will be testing on

## linux.

## the app has some sort of tab bar, it can be as basic as you want.

## an input box to enter url, again as basic as you want

## the actual JSML page rendered


