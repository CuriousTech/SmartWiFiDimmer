const char page1[] PROGMEM =
   "<!DOCTYPE html>\n"
   "<html lang=\"en\">\n"
   "<head>\n"
   "<meta name=\"viewport\" content=\"width=device-width, initial-scale=1\"/>\n"
   "<title>WiFi Dimmer</title>\n"
   "<style>div,input{margin-bottom: 5px;}\n"
   "table,input{border-radius: 5px;box-shadow: 2px 2px 12px #000000;\n"
   "background-image: -moz-linear-gradient(top, #ffffff, #50a0ff);\n"
   "background-image: -ms-linear-gradient(top, #ffffff, #50a0ff);\n"
   "background-image: -o-linear-gradient(top, #ffffff, #50a0ff);\n"
   "background-image: -webkit-linear-gradient(top, #efffff, #50a0ff);\n"
   "background-image: linear-gradient(top, #ffffff, #50a0ff);\n"
   "background-clip: padding-box;}\n"
   "body{width:470px;display:block;margin-left:auto;margin-right:auto;text-align:right;font-family: Arial, Helvetica, sans-serif;}\n"
   "</style>\n"
   "<script type=\"text/javascript\" src=\"data\"></script>\n"
   "<script type=\"text/javascript\">\n"
   "a=document.all\n"
   "idx=0\n"
   "function openSocket(){\n"
   "ws=new WebSocket(\"ws://\"+window.location.host+\"/ws\")\n"
   "//ws=new WebSocket(\"ws://192.168.0.114/ws\")\n"
   "ws.onopen=function(evt){}\n"
   "ws.onclose=function(evt){alert(\"Connection closed.\");}\n"
   "ws.onmessage=function(evt){\n"
   "//console.log(evt.data)\n"
   " lines=evt.data.split(';')\n"
   " event=lines[0]\n"
   " data=lines[1]\n"
   " if(event=='state'){\n"
   " d=JSON.parse(data)\n"
   "d2=new Date(d.t*1000)\n"
   "days=[['Sun'],['Mon'],['Tue'],['Wed'],['Thu'],['Fri'],['Sat']]\n"
   "a.time.innerHTML=days[d2.getDay()]+' '+d2.toLocaleTimeString()+' T:'+t2hms(d.tr)\n"
   "a.RLY.value=d.on?' ON ':'OFF'\n"
   "a.RLY.setAttribute('style',d.on?'color:red':'')\n"
   "a.LED1.value=d.l1?' ON ':'OFF'\n"
   "a.LED1.setAttribute('style',d.l1?'color:green':'')\n"
   "a.LVL.value=d.lvl\n"
   "a.level.value=d.lvl\n"
   "a.MOT.setAttribute('bgcolor',d.mo?'red':'')\n"
   "if(d.sn) document.getElementById('r'+(d.sn-1)).setAttribute('bgcolor','red')\n"
   " }\n"
   " else if(event=='set'){}\n"
   " else if(event=='alert'){alert(data)}\n"
   "}\n"
   "}\n"
   "function setVar(varName, value)\n"
   "{\n"
   " ws.send('cmd;{\"key\":\"'+a.myKey.value+'\",\"'+varName+'\":'+value+'}')\n"
   "}\n"
   "function manual()\n"
   "{\n"
   "on=(a.RLY.value=='OFF')\n"
   "setVar('on',on)\n"
   "a.RLY.value=on?' ON ':'OFF'\n"
   "a.RLY.setAttribute('style',on?'color:red':'')\n"
   "}\n"
   "\n"
   "function led1()\n"
   "{\n"
   "on=(a.LED1.value=='OFF')\n"
   "setVar('led1',on)\n"
   "a.LED1.value=on?' ON ':'OFF'\n"
   "a.LED1.setAttribute('style',on?'color:blue':'')\n"
   "}\n"
   "\n"
   "function t2hms(t)\n"
   "{\n"
   "s=t%60\n"
   "t=Math.floor(t/60)\n"
   "if(t==0) return s\n"
   "if(s<10) s='0'+s\n"
   "m=t%60\n"
   "t=Math.floor(t/60)\n"
   "if(t==0) return m+':'+s\n"
   "if(m<10) m='0'+m\n"
   "return t+':'+m+':'+s\n"
   "}\n"
   "\n"
   "function hms2t(s)\n"
   "{\n"
   "ar=s.split(':')\n"
   "t=0\n"
   "for(i=0;i<ar.length;i++){t*=60;t+=+ar[i]}\n"
   "return t\n"
   "}\n"
   "\n"
   "function setData()\n"
   "{\n"
   "  for(it=0;it<item.length;it++)\n"
   "  {\n"
   "   wks=document.getElementsByName('W'+it)\n"
   "   w=0\n"
   "   for(wd=0;wd<10;wd++) if(wks[wd].checked) w|=(1<<wd)\n"
   "   save=false\n"
   "   if(item[it][0]!=document.getElementById('N'+it).value) save=true\n"
   "   if(item[it][1]!=hms2t(document.getElementById('S'+it).value)) save=true\n"
   "   if(item[it][2]!=hms2t(document.getElementById('T'+it).value)) save=true\n"
   "   if(item[it][3]!=w) save=true\n"
   "   if(item[it][4]!=document.getElementById('L'+it).value) save=true\n"
   "   if(save)\n"
   "   {\n"
   "    item[it][0]=document.getElementById('N'+it).value\n"
   "    item[it][1]=hms2t(document.getElementById('S'+it).value)\n"
   "    item[it][2]=hms2t(document.getElementById('T'+it).value)\n"
   "    item[it][3]=w\n"
   "    item[it][4]=document.getElementById('L'+it).value\n"
   "\n"
   "    s='cmd;{\"key\":\"'+a.myKey.value+'\",\"I\":'+it\n"
   "    s+=',\"N\":\"'+item[it][0]+'\"'\n"
   "    s+=',\"S\":'+item[it][1]\n"
   "    s+=',\"T\":'+item[it][2]\n"
   "    s+=',\"W\":'+item[it][3]\n"
   "    s+=',\"L\":'+item[it][4]\n"
   "    s+='}'\n"
   "    ws.send(s)\n"
   "   }\n"
   "  }\n"
   "  if(item[item.length-1][0]!='')\n"
   "  {\n"
   "   item.push(['',60,60,255])\n"
   "   AddEntry(item[item.length-1])\n"
   "  }\n"
   "  setVar('MOT',hms2t(a.mtime.value))\n"
   "  setVar('auto',hms2t(a.auto.value))\n"
   "  setVar('TZ',a.TZ.value)\n"
   "}\n"
   "\n"
   "function togCall()\n"
   "{\n"
   "  on=(a.CH.value=='OFF')\n"
   "  setVar('ch',on)\n"
   "  a.CH.value=on?' ON ':'OFF'\n"
   "  a.CH.setAttribute('style',on?'color:red':'')\n"
   "}\n"
   "\n"
   "function slide()\n"
   "{\n"
   " lvl=a.level.value\n"
   " a.LVL.value=lvl\n"
   " setVar('lvl',lvl)\n"
   "}\n"
   "\n"
   "function setDelay()\n"
   "{\n"
   "  setVar('dly',hms2t(a.dly.value))\n"
   "}\n"
   "\n"
   "function setAuto()\n"
   "{\n"
   "  setVar('auto',hms2t(a.auto.value))\n"
   "}\n"
   "\n"
   "function fillData()\n"
   "{\n"
   "  a.TZ.value=data.tz\n"
   "  a.mtime.value=t2hms(data.mot)\n"
   "  a.auto.value=t2hms(data.auto)\n"
   "  a.CH.value=data.ch?' ON ':'OFF'\n"
   "  a.CH.setAttribute('style',data.ch?'color:red':'')\n"
   "  for(it=0;it<item.length;it++)\n"
   "  AddEntry(item[it])\n"
   "}\n"
   "\n"
   "function AddEntry(arr)\n"
   "{\n"
   "  td=document.createElement(\"td\")\n"
   "  inp=document.createElement(\"input\")\n"
   "  inp.id='N'+idx\n"
   "  inp.type='text'\n"
   "  inp.size=9\n"
   "  inp.value=arr[0]\n"
   "  td.appendChild(inp)\n"
   "\n"
   "  for(i=0;i<10;i++){\n"
   "inp=document.createElement(\"input\")\n"
   "inp.type='checkbox'\n"
   "inp.name='W'+idx\n"
   "inp.checked=(arr[3]&(1<<i))\n"
   "td.appendChild(inp)\n"
   "  }\n"
   "\n"
   "  inp=document.createElement(\"input\")\n"
   "  inp.id='S'+idx\n"
   "  inp.type='text'\n"
   "  inp.value=t2hms(arr[1])\n"
   "  inp.size=3\n"
   "  td.appendChild(inp)\n"
   "\n"
   "  inp=document.createElement(\"input\")\n"
   "  inp.id='T'+idx\n"
   "  inp.type='text'\n"
   "  t=t2hms(arr[2])\n"
   "  inp.value=(t)?t:''\n"
   "  inp.size=2\n"
   "  td.appendChild(inp)\n"
   "\n"
   "  inp=document.createElement(\"input\")\n"
   "  inp.id='L'+idx\n"
   "  inp.type='text'\n"
   "  inp.value=arr[4]?arr[4]:''\n"
   "  inp.size=2\n"
   "  td.appendChild(inp)\n"
   "\n"
   "  tr=document.createElement(\"tr\")\n"
   "  tr.id='r'+idx\n"
   "  tr.appendChild(td)\n"
   "  tbody=document.createElement(\"tbody\")\n"
   "  tbody.id='tbody'+idx\n"
   "  tbody.appendChild(tr)\n"
   "  a.list.appendChild(tbody)\n"
   "  idx++\n"
   "}\n"
   "\n"
   "</script>\n"
   "<body bgcolor=\"silver\" onload=\"{\n"
   "key=localStorage.getItem('key')\n"
   "if(key!=null) document.getElementById('myKey').value=key\n"
   "openSocket()\n"
   "fillData()\n"
   "}\"\n"
   ">\n"
   "<table align=\"right\" width=\"450\">\n"
   "<tr><td>Light</td><td>LED &nbsp Report</td><td><div id=\"time\"></div></td></tr>\n"
   "<tr><td><input name=\"RLY\" value=\"OFF\" type='button' onclick=\"{manual()}\"></td>\n"
   "<td><input name=\"LED1\" value=\"OFF\" type='button' onclick=\"{led1()}\">&nbsp <input name=\"CH\" value=\"OFF\" type='button' onclick=\"{togCall()}\"></td>\n"
   "<td><label for=\"auto\">Auto Off </label><input name=\"auto\" type=\"text\" value=\"0\" size=\"4\"> &nbsp &nbsp <label for=\"TZ\">TZ </label><input name=\"TZ\" type=text size=1 value='-5'></td></tr>\n"
   "<tr>\n"
   "<td colspan=\"3\"><label for=\"LVL\">Level </label><input name=\"LVL\" value=\"255\" type='text' size=\"2\"> &nbsp <input type=\"range\" id=\"level\" name=\"level\" min=\"0\" max=\"255\" onchange=\"slide();\">  &nbsp\n"
   " &nbsp <input name=\"dly\" type=\"text\" value=\"0\" size=\"4\"><input value=\"Delay On\" type='button' onclick=\"{setDelay()}\"></td></tr>\n"
   "\n"
   "<tr><td>Name &nbsp </td><td colspan=2><small>En Su Mo Tu We Th Fr Sa On Off</small>&nbsp Time &nbsp Duration Level</td></tr>\n"
   "</table>\n"
   "<table align=\"right\" style=\"font-size:small\" id=\"list\"></table>\n"
   "<table align=\"right\" width=\"450\">\n"
   "<tr><td id=\"MOT\">Motion</td><td><input name=\"mtime\" type=text size=4 style=\"width: 60px\"> &nbsp\n"
   " <input value='Save Changes' type=button onclick=\"setData();\"> &nbsp </td><td> <input id=\"myKey\" name=\"key\" type=text size=50 placeholder=\"password\" style=\"width: 128px\">\n"
   "<input type=\"button\" value=\"Set\" onClick=\"{localStorage.setItem('key', key = document.all.myKey.value)}\"></td></tr>\n"
   "</table>\n"
   "</body>\n"
   "</html>\n";

const uint8_t favicon[] PROGMEM = {
  0x1F, 0x8B, 0x08, 0x08, 0x70, 0xC9, 0xE2, 0x59, 0x04, 0x00, 0x66, 0x61, 0x76, 0x69, 0x63, 0x6F, 
  0x6E, 0x2E, 0x69, 0x63, 0x6F, 0x00, 0xD5, 0x94, 0x31, 0x4B, 0xC3, 0x50, 0x14, 0x85, 0x4F, 0x6B, 
  0xC0, 0x52, 0x0A, 0x86, 0x22, 0x9D, 0xA4, 0x74, 0xC8, 0xE0, 0x28, 0x46, 0xC4, 0x41, 0xB0, 0x53, 
  0x7F, 0x87, 0x64, 0x72, 0x14, 0x71, 0xD7, 0xB5, 0x38, 0x38, 0xF9, 0x03, 0xFC, 0x05, 0x1D, 0xB3, 
  0x0A, 0x9D, 0x9D, 0xA4, 0x74, 0x15, 0x44, 0xC4, 0x4D, 0x07, 0x07, 0x89, 0xFA, 0x3C, 0x97, 0x9C, 
  0xE8, 0x1B, 0xDA, 0x92, 0x16, 0x3A, 0xF4, 0x86, 0x8F, 0x77, 0x73, 0xEF, 0x39, 0xEF, 0xBD, 0xBC, 
  0x90, 0x00, 0x15, 0x5E, 0x61, 0x68, 0x63, 0x07, 0x27, 0x01, 0xD0, 0x02, 0xB0, 0x4D, 0x58, 0x62, 
  0x25, 0xAF, 0x5B, 0x74, 0x03, 0xAC, 0x54, 0xC4, 0x71, 0xDC, 0x35, 0xB0, 0x40, 0xD0, 0xD7, 0x24, 
  0x99, 0x68, 0x62, 0xFE, 0xA8, 0xD2, 0x77, 0x6B, 0x58, 0x8E, 0x92, 0x41, 0xFD, 0x21, 0x79, 0x22, 
  0x89, 0x7C, 0x55, 0xCB, 0xC9, 0xB3, 0xF5, 0x4A, 0xF8, 0xF7, 0xC9, 0x27, 0x71, 0xE4, 0x55, 0x38, 
  0xD5, 0x0E, 0x66, 0xF8, 0x22, 0x72, 0x43, 0xDA, 0x64, 0x8F, 0xA4, 0xE4, 0x43, 0xA4, 0xAA, 0xB5, 
  0xA5, 0x89, 0x26, 0xF8, 0x13, 0x6F, 0xCD, 0x63, 0x96, 0x6A, 0x5E, 0xBB, 0x66, 0x35, 0x6F, 0x2F, 
  0x89, 0xE7, 0xAB, 0x93, 0x1E, 0xD3, 0x80, 0x63, 0x9F, 0x7C, 0x9B, 0x46, 0xEB, 0xDE, 0x1B, 0xCA, 
  0x9D, 0x7A, 0x7D, 0x69, 0x7B, 0xF2, 0x9E, 0xAB, 0x37, 0x20, 0x21, 0xD9, 0xB5, 0x33, 0x2F, 0xD6, 
  0x2A, 0xF6, 0xA4, 0xDA, 0x8E, 0x34, 0x03, 0xAB, 0xCB, 0xBB, 0x45, 0x46, 0xBA, 0x7F, 0x21, 0xA7, 
  0x64, 0x53, 0x7B, 0x6B, 0x18, 0xCA, 0x5B, 0xE4, 0xCC, 0x9B, 0xF7, 0xC1, 0xBC, 0x85, 0x4E, 0xE7, 
  0x92, 0x15, 0xFB, 0xD4, 0x9C, 0xA9, 0x18, 0x79, 0xCF, 0x95, 0x49, 0xDB, 0x98, 0xF2, 0x0E, 0xAE, 
  0xC8, 0xF8, 0x4F, 0xFF, 0x3F, 0xDF, 0x58, 0xBD, 0x08, 0x25, 0x42, 0x67, 0xD3, 0x11, 0x75, 0x2C, 
  0x29, 0x9C, 0xCB, 0xF9, 0xB9, 0x00, 0xBE, 0x8E, 0xF2, 0xF1, 0xFD, 0x1A, 0x78, 0xDB, 0x00, 0xEE, 
  0xD6, 0x80, 0xE1, 0x90, 0xFF, 0x90, 0x40, 0x1F, 0x04, 0xBF, 0xC4, 0xCB, 0x0A, 0xF0, 0xB8, 0x6E, 
  0xDA, 0xDC, 0xF7, 0x0B, 0xE9, 0xA4, 0xB1, 0xC3, 0x7E, 0x04, 0x00, 0x00, 
};
