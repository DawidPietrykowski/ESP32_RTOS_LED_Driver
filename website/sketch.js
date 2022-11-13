let led_num = 60; 
let width = window.innerWidth-120;
let height = 50;
let strip = [[]];
let anim_num = 0;
let counter = 0;
let breathing_counter = 0
let last_time = 0;
let solid_color = {
  r : 255,
  g : 0,
  b : 12
};
let domain_name = window.location.href;

let config = {
  "mode": "rainbow",
  "anim_refresh_rate": 60,
  "refresh_rate": 60,
  "brightness": 255,
  "brightness_step": 64,
  "default_animation": 0,
  "enable_touch_control": true,
  "selected_color":"0000ff",
  "spread_value":3,
  "animation_direction":1,
  "single_color": true,
  "anim_speed":100
};

initConfig();

function initConfig(){
  var xmlHttp = new XMLHttpRequest();
  xmlHttp.onreadystatechange = function() { 
      if (xmlHttp.readyState == 4 && xmlHttp.status == 200){
        console.log(xmlHttp.responseText);
        obj = JSON.parse(xmlHttp.responseText);
        config["mode"] = obj["mode"];
        config["anim_refresh_rate"] = obj["anim_refresh_rate"];
        config["refresh_rate"] = obj["refresh_rate"];
        config["brightness"] = obj["brightness"];
        config["brightness_step"] = obj["brightness_step"];
        config["default_animation"] = obj["default_animation"];
        config["enable_touch_control"] = obj["enable_touch_control"];
        config["anim_speed"] = obj["anim_speed"];
        config["spread_value"] = 3;
        config["animation_direction"] = 1;
        config["single_color"] = false;
        console.log(config);
  }
}
  
  xmlHttp.open( "GET", domain_name + "config", true );
  xmlHttp.send( null );
  return xmlHttp.responseText;
}

function HSVtoRGB(h, s, v) {
  var r, g, b, i, f, p, q, t;
  if (arguments.length === 1) {
      s = h.s, v = h.v, h = h.h;
  }
  i = Math.floor(h * 6);
  f = h * 6 - i;
  p = v * (1 - s);
  q = v * (1 - f * s);
  t = v * (1 - (1 - f) * s);
  switch (i % 6) {
      case 0: r = v, g = t, b = p; break;
      case 1: r = q, g = v, b = p; break;
      case 2: r = p, g = v, b = t; break;
      case 3: r = p, g = q, b = v; break;
      case 4: r = t, g = p, b = v; break;
      case 5: r = v, g = p, b = q; break;
  }
  return {
      r: Math.round(r * 255),
      g: Math.round(g * 255),
      b: Math.round(b * 255)
  };
}

let animation_texts = ["rainbow", "static", "breathing", "blinking", "chase", "police", "sparkles", "off", "received"];

let dark_color_picker = 
    new ColorPickerControl({ container: document.querySelector('.color-picker-dark-theme'), theme: 'dark' });

dark_color_picker.on('change', (color) =>  {
    // document.getElementById("butterfly").style.setProperty('--butterfly-color', color.toHEX());
    // document.getElementById("butterfly").style.setProperty('--butterfly-opacity', color.a / 255);
    // light_color_picker.color.fromHSVa(color.h, color.s, color.v, color.a);
    selectColor(color);
});

function selectColor(color){
  
  console.log(color.toRGB());
  solid_color.r = color.toRGB()[0];
  solid_color.g = color.toRGB()[1];
  solid_color.b = color.toRGB()[2];
  config["selected_color"] = color.toHEX().substring(1, 7).toLowerCase();
  console.log(config["selected_color"]);
}

function selectAnimation(animation_number){
  anim_num = animation_number;
  var element = document.getElementById("animation-refresh-value");
  config["anim_refresh_rate"] = parseInt(element.textContent);
  var element = document.getElementById("refresh-rate-value");
  config["refresh_rate"] = parseInt(element.textContent);
  var element = document.getElementById("brightness-value");
  config["brightness"] = parseInt(element.textContent);
  var element = document.getElementById("brightness-step-value");
  config["brightness_step"] = parseInt(element.textContent);
  var element = document.getElementById("default-animation-value");
  config["default_animation"] = parseInt(element.textContent);
  var element = document.getElementById("touch-control-value");
  config["enable_touch_control"] = element.checked;

  var element = document.getElementById("animation-speed-value");
  config["anim_speed"] = parseInt(element.textContent);
  var element = document.getElementById("animation-spread-value");
  config["spread_value"] = parseInt(element.textContent);
  var element = document.getElementById("animation-direction-value");
  config["animation_direction"] = parseInt(element.textContent);
  var element = document.getElementById("single-color-value");
  config["single_color"] = element.checked;

  var xhr = new XMLHttpRequest();
  var url = domain_name;
  //var url = "http://192.168.1.89";
  xhr.open("POST", url, true);
  xhr.setRequestHeader("Content-Type", "application/json");
  xhr.onreadystatechange = function () {
      if (xhr.readyState === 4 && xhr.status === 200) {
          console.log("received HTTP 200");
          // var json = JSON.parse(xhr.responseText);
          // console.log(json.email + ", " + json.password);
      }
  };

  config["mode"] = animation_texts[animation_number];
  var data = JSON.stringify(config);
  console.log("sending data");
  console.log(data);
  xhr.send(data);

  for (const element of document.getElementsByClassName("menu-entry")) {
    if(element.id == animation_number)
      element.classList.add("active");
    else
      element.classList.remove("active");
}
}

function apply_animation(){
  switch(anim_num){
    case 0:
      counter += ((millis()-last_time)/1000)*60;
      last_time = millis();
      for(let i = 0; i < led_num; i++){
        let obj = HSVtoRGB(i/led_num+(counter/360.0), 1, 1);
        strip[i] = [obj.r, obj.g, obj.b];
      }
      break;
    case 1:
      for(let i = 0; i < led_num; i++){
        strip[i] = [solid_color.r, solid_color.g, solid_color.b];
      }
      break;
    case 2:
      breathing_counter += ((millis()-last_time)/1000)*0.1*2*Math.PI;
      multiplier = (1 + sin(breathing_counter)) / 2
      last_time = millis();
      for(let i = 0; i < led_num; i++){
        strip[i] = [solid_color.r * multiplier, solid_color.g * multiplier, solid_color.b * multiplier];
      }
      break;
  }

}

function setup() {
    var canvas = createCanvas(width, height);
    canvas.parent('canvas_container');
    last_time=millis();
  }
function draw() {
    clear(18,18,18);
    apply_animation();
    for(let i = 0; i < led_num; i++){
      fill(strip[i][0],strip[i][1],strip[i][2]);
      rect(i*(width/led_num), 0, (i+1)*(width/led_num), height);
    }
  }  