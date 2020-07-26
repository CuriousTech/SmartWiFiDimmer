const char page1[] PROGMEM =
   "<!DOCTYPE html>\n"
   "<html lang=\"en\">\n"
   "<head>\n"
   "<meta name=\"viewport\" content=\"width=device-width, initial-scale=1\"/>\n"
   "<title>WiFi Dimmer</title>\n"
   "<style>div,input{margin-bottom: 5px;}\n"
   "table{border-radius: 5px;box-shadow: 2px 2px 12px #000000;\n"
   "background-image: -moz-linear-gradient(top, #ffffff, #4080ff);\n"
   "background-image: -ms-linear-gradient(top, #ffffff, #4080ff);\n"
   "background-image: -o-linear-gradient(top, #ffffff, #4080ff);\n"
   "background-image: -webkit-linear-gradient(top, #ffffff, #4080ff);\n"
   "background-image: linear-gradient(top, #ffffff, #4080ff);\n"
   "background-clip: padding-box;}\n"
   "input{border-radius: 5px;box-shadow: 2px 2px 12px #000000;\n"
   "background-image: -moz-linear-gradient(top, #ffffff, #505050);\n"
   "background-image: -ms-linear-gradient(top, #ffffff, #505050);\n"
   "background-image: -o-linear-gradient(top, #ffffff, #505050);\n"
   "background-image: -webkit-linear-gradient(top, #ffffff, #2070a0);\n"
   "background-image: linear-gradient(top, #ffffff, #505050);\n"
   "background-clip: padding-box;}\n"
   "body{width:470px;display:block;margin-left:auto;margin-right:auto;text-align:right;font-family: Arial, Helvetica, sans-serif;}\n"
   "</style>\n"
   "<script src=\"http://ajax.googleapis.com/ajax/libs/jquery/1.6.1/jquery.min.js\" type=\"text/javascript\" charset=\"utf-8\"></script>\n"
   "<script type=\"text/javascript\" src=\"data\"></script>\n"
   "<script type=\"text/javascript\">\n"
   "a=document.all\n"
   "idx=0\n"
   "nms=['OFF','ON ','LNK','REV']\n"
   "dys=[['Sun'],['Mon'],['Tue'],['Wed'],['Thu'],['Fri'],['Sat']]\n"
   "maxW=0\n"
   "wArr=[]\n"
   "function openSocket(){\n"
   "ws=new WebSocket(\"ws://\"+window.location.host+\"/ws\")\n"
   "//ws=new WebSocket(\"ws://192.168.0.122/ws\")\n"
   "ws.onopen=function(evt){}\n"
   "ws.onclose=function(evt){alert(\"Connection closed.\");}\n"
   "ws.onmessage=function(evt){\n"
   "console.log(evt.data)\n"
   " lines=evt.data.split(';')\n"
   " event=lines[0]\n"
   " data=lines[1]\n"
   " if(event=='state'){\n"
   "d=JSON.parse(data)\n"
   "d2=new Date(d.t*1000)\n"
   "a.time.innerHTML=dys[d2.getDay()]+' '+d2.toLocaleTimeString()+' T:'+t2hms(d.tr)\n"
   "a.RLY.value=nms[d.on]\n"
   "a.RLY.setAttribute('style',d.on?'color:red':'')\n"
   "a.LED1.value=nms[d.l1]\n"
   "a.LED1.setAttribute('style',d.l1?'color:blue':'')\n"
   "a.LED2.value=nms[d.l2]\n"
   "a.LED2.setAttribute('style',d.l2?'color:blue':'')\n"
   "a.LVL.value=d.lvl\n"
   "a.level.value=d.lvl\n"
   "rs=d.rssi+'dB '\n"
   "if(d.v) rs += '&nbsp;&nbsp;'+d.v+'V'\n"
   "a.rssi.innerHTML=rs\n"
   "if(d.c) a.v.innerHTML='&nbsp;&nbsp;'+d.c+'A'\n"
   "  if(d.p) a.watts.value=d.p\n"
   "\n"
   "a.ts.innerHTML=t2hms(d.ts)+' &nbsp; Since ' + (new Date(d.st*1000)).toLocaleString()\n"
   "if(d.lvl==0)\n"
   "{\n"
   " a.LVL.style.visibility='hidden'\n"
   " a.level.style.visibility='hidden'\n"
   " for(i=0;i<item.length;i++)\n"
   "  document.getElementById('L'+i).style.visibility='hidden'\n"
   " a.header.innerHTML='<small>En Su Mo Tu We Th Fr Sa On Off</small> Time Duration &nbsp; &nbsp; &nbsp; &nbsp; &nbsp; '\n"
   "}\n"
   "a.wh.value=d.wh.toFixed(2)\n"
   "a.cost.value=((+d.wh)*(+a.ppkw.value/1000)).toFixed(3)\n"
   "a.MOT.setAttribute('bgcolor',d.mo?'red':'')\n"
   "if(d.sn) document.getElementById('r'+(d.sn-1)).setAttribute('bgcolor','red')\n"
   " }\n"
   " else if(event=='energy')\n"
   " {\n"
   "d=JSON.parse(data)\n"
   "d2=new Date(d.t*1000)\n"
   "a.time.innerHTML=dys[d2.getDay()]+' '+d2.toLocaleTimeString()+' T:'+t2hms(d.tr)\n"
   "rs=d.rssi+'dB &nbsp;&nbsp;'+d.v+'V'\n"
   "a.rssi.innerHTML=rs\n"
   "a.v.innerHTML='&nbsp;&nbsp;'+d.c+'A'\n"
   "  a.watts.value=d.p\n"
   "a.wh.value=d.wh.toFixed(2)\n"
   "draw_point(d.p)\n"
   " }\n"
   " else if(event=='alert'){alert(data)}\n"
   "// draw_chart()\n"
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
   "a.RLY.value=on?'ON ':'OFF'\n"
   "a.RLY.setAttribute('style',on?'color:red':'')\n"
   "}\n"
   "\n"
   "function pf()\n"
   "{\n"
   "if(a.PF.value=='LAST') l=1\n"
   "else if(a.PF.value=='OFF ') l=2\n"
   "else l=0\n"
   "setVar('powerup',l)\n"
   "pus=['LAST','OFF ',' ON ']\n"
   "a.PF.value=pus[l]\n"
   "}\n"
   "\n"
   "function led1()\n"
   "{\n"
   "if(a.LED1.value=='OFF') l=1\n"
   "else if(a.LED1.value=='ON ') l=2\n"
   "else if(a.LED1.value=='LNK') l=3\n"
   "else l=0\n"
   "setVar('led1',l)\n"
   "a.LED1.value=nms[l]\n"
   "a.LED1.setAttribute('style',l?'color:blue':'')\n"
   "}\n"
   "function led2()\n"
   "{\n"
   "if(a.LED2.value=='OFF') l=1\n"
   "else if(a.LED2.value=='ON ') l=2\n"
   "else if(a.LED2.value=='LNK') l=3\n"
   "else l=0\n"
   "setVar('led2',l)\n"
   "a.LED2.value=nms[l]\n"
   "a.LED2.setAttribute('style',l?'color:blue':'')\n"
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
   "h=t%24\n"
   "t=Math.floor(t/24)\n"
   "if(t==0) return h+':'+m+':'+s\n"
   "return t+'d '+h+':'+m+':'+s\n"
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
   "  s='cmd;{\"key\":\"'+a.myKey.value+'\",'\n"
   "  s+='\"MOT\":' +hms2t(a.mtime.value)+','\n"
   "  s+='\"auto\":'+hms2t(a.auto.value)+','\n"
   "  s+='\"TZ\":'+a.TZ.value+','\n"
   "  s+='\"watts\":'+a.watts.value+','\n"
   "  s+='\"ppkw\":'+(+a.ppkw.value)*1000+'}'\n"
   "  ws.send(s)\n"
   "}\n"
   "\n"
   "function togCall()\n"
   "{\n"
   "  on=(a.CH.value=='OFF')?1:0\n"
   "  setVar('ch',on)\n"
   "  a.CH.value=nms[on]\n"
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
   "function clearWh()\n"
   "{\n"
   "  setVar('rt',0)\n"
   "  a.cost.value='0.000'\n"
   "  a.wh.value='0.00'\n"
   "}\n"
   "\n"
   "function changeNm()\n"
   "{\n"
   "  setVar('name',a.nm.value)\n"
   "}\n"
   "\n"
   "function fillData()\n"
   "{\n"
   "  a.TZ.value=data.tz\n"
   "  a.nm.value=data.name\n"
   "  a.mtime.value=t2hms(data.mot)\n"
   "  a.auto.value=t2hms(data.auto)\n"
   "  a.CH.value=data.ch?' ON ':'OFF'\n"
   "  pus=['LAST','OFF ',' ON ']\n"
   "  a.PF.value=pus[data.pu]\n"
   "  a.CH.setAttribute('style',data.ch?'color:red':'')\n"
   "  a.watts.value=data.watt\n"
   "  a.ppkw.value=data.ppkw/1000\n"
   "  for(it=0;it<item.length;it++)\n"
   "  AddEntry(item[it])\n"
   "  idx=0\n"
   "  for(d=0;d<dev.length;d++)\n"
   "  AddDev(dev[d])\n"
   "}\n"
   "\n"
   "function AddEntry(arr)\n"
   "{\n"
   "  td=document.createElement(\"td\")\n"
   "  inp=document.createElement(\"input\")\n"
   "  inp.id='N'+idx\n"
   "  inp.type='text'\n"
   "  inp.size=8\n"
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
   "  inp.size=1\n"
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
   "function changeDev(dev)\n"
   "{\n"
   " idx=dev.id.split('-')\n"
   " val=document.getElementById(dev.id).value\n"
   " switch(idx[0])\n"
   " {\n"
   " case 'DE':\n"
   "   s=document.getElementById('IP-'+idx[1]).innerHTML\n"
   "   window.location.href='http://'+s\n"
   "   break\n"
   " case 'ON':\n"
   "   mode=0\n"
   "      if(val=='------'){ document.getElementById(dev.id).value='LNK'; mode=1}\n"
   "      else if(val=='LNK'){document.getElementById(dev.id).value='REV'; mode=2}\n"
   "   else document.getElementById(dev.id).value='------'\n"
   "  setVar('DEV',idx[1])\n"
   "  setVar('MODE',mode)\n"
   "   break\n"
   " case 'DM':\n"
   "  setVar('DEV',idx[1])\n"
   "  setVar('DIM',(document.getElementById(dev.id).checked)?1:0)\n"
   "   break\n"
   " case 'OP':\n"
   "  setVar('DEV',idx[1])\n"
   "  setVar('OP',(document.getElementById(dev.id).checked)?1:0)\n"
   "   break\n"
   " case 'FO':\n"
   "  setVar('DEV',idx[1])\n"
   "  setVar('FO',(document.getElementById(dev.id).checked)?1:0)\n"
   "   break\n"
   " case 'DLY':\n"
   "  setVar('DEV',idx[1])\n"
   "  setVar('DLY',hms2t(val))\n"
   "   break\n"
   " }\n"
   "}\n"
   "\n"
   "// \"name\",\"192.168.0.122\",mode,flags,dly\n"
   "function AddDev(arr)\n"
   "{\n"
   "  td=document.createElement(\"td\")\n"
   "  td.align=\"center\"\n"
   "\n"
   "  inp=document.createElement(\"input\")\n"
   "  inp.id='DE-'+idx\n"
   "  inp.type='button'\n"
   "  inp.style.width='100px'\n"
   "  inp.value=arr[0]\n"
   "  inp.onclick=function(){changeDev(this); }\n"
   "  td.appendChild(inp)\n"
   "\n"
   "  inp=document.createElement(\"button\")\n"
   "  inp.id='IP-'+idx\n"
   "  inp.border=1\n"
   "  inp.type='text'\n"
   "  inp.innerHTML=arr[1]\n"
   "  td.appendChild(inp)\n"
   "\n"
   "  inp=document.createElement(\"input\")\n"
   "  inp.id='ON-'+idx\n"
   "  inp.type='button'\n"
   "  nams=['------','LNK','REV']\n"
   "  inp.value=nams[arr[2]]\n"
   "  inp.onclick=function(){changeDev(this); }\n"
   "  td.appendChild(inp)\n"
   "\n"
   "  inp=document.createElement(\"input\")\n"
   "  inp.id='DM-'+idx\n"
   "  inp.type='checkbox'\n"
   "  inp.checked=(arr[3]&1)?1:0\n"
   "  inp.onclick=function(){changeDev(this); }\n"
   "  td.appendChild(inp)\n"
   "\n"
   "  inp=document.createElement(\"input\")\n"
   "  inp.id='OP-'+idx\n"
   "  inp.type='checkbox'\n"
   "  inp.checked=(arr[3]&2)?1:0\n"
   "  inp.onclick=function(){changeDev(this); }\n"
   "  td.appendChild(inp)\n"
   "\n"
   "  inp=document.createElement(\"input\")\n"
   "  inp.id='FO-'+idx\n"
   "  inp.type='checkbox'\n"
   "  inp.checked=(arr[3]&4)?1:0\n"
   "  inp.onclick=function(){changeDev(this); }\n"
   "  td.appendChild(inp)\n"
   "\n"
   "  inp=document.createElement(\"input\")\n"
   "  inp.id='DLY-'+idx\n"
   "  inp.type='text'\n"
   "  inp.size=3\n"
   "  inp.value=t2hms(arr[4])\n"
   "  inp.onchange=function(){changeDev(this); }\n"
   "  td.appendChild(inp)\n"
   "\n"
   "  tr=document.createElement(\"tr\")\n"
   "  tr.id='dv'+idx\n"
   "  tr.appendChild(td)\n"
   "  tbody=document.createElement(\"tbody\")\n"
   "  tbody.id='tbody'+idx\n"
   "  tbody.appendChild(tr)\n"
   "  a.dev.appendChild(tbody)\n"
   "  idx++\n"
   "}\n"
   "\n"
   "function draw_point(wt){\n"
   "try {\n"
   "var c=document.getElementById('chart')\n"
   "ctx=c.getContext(\"2d\")\n"
   "ctx.fillStyle=\"#225\"\n"
   "ht=(c.height/4)-5\n"
   "ctx.fillRect(0,0,c.width,ht)\n"
   "\n"
   "    date = new Date()\n"
   "if(wt>maxW) maxW=wt\n"
   "ctx.fillStyle=\"#FFF\"\n"
   "ctx.textAlign=\"right\"\n"
   "ctx.fillText(maxW,c.width-1,8)\n"
   "i=date.getMinutes()*60+date.getSeconds()\n"
   "wArr[i]=wt\n"
   "ctx.strokeStyle=\"#EEE\"\n"
   "ctx.lineWidth=0.4\n"
   "ctx.beginPath()\n"
   "x=i*c.width/3600\n"
   "ctx.moveTo(x,0)\n"
   "ctx.lineTo(x,ht)\n"
   "ctx.stroke()\n"
   "ctx.strokeStyle=\"#F55\"\n"
   "ctx.lineWidth=1.5\n"
   "ctx.beginPath()\n"
   "mvd=0\n"
   "for(i=0;i<3600;i++)\n"
   "{\n"
   "if(wArr[i])\n"
   "{\n"
   "x=i*c.width/3600\n"
   "    y=wArr[i]*ht/maxW\n"
   "if(!mvd) ctx.moveTo(x,ht-y)\n"
   "else ctx.lineTo(x,ht-y)\n"
   "mvd=true\n"
   "}\n"
   "}\n"
   "ctx.stroke()\n"
   "}catch(err){console.log(err)}\n"
   "}\n"
   "$(document).ready(function(){\n"
   "try {\n"
   "key=localStorage.getItem('key')\n"
   "if(key!=null) document.getElementById('myKey').value=key\n"
   "openSocket()\n"
   "fillData()\n"
   "\n"
   "    graph = $('#chart')\n"
   "var c=document.getElementById('chart')\n"
   "rect=c.getBoundingClientRect()\n"
   "canvasX=rect.x\n"
   "canvasY=rect.y\n"
   "\n"
   "    tipCanvas=document.getElementById(\"tip\")\n"
   "    tipCtx=tipCanvas.getContext(\"2d\")\n"
   "    tipDiv=document.getElementById(\"popup\")\n"
   "\n"
   "ctx=c.getContext(\"2d\")\n"
   "ctx.fillStyle=\"#336\"\n"
   "ctx.fillRect(0,0,c.width,c.height)\n"
   "ctx.fillStyle=\"#FFF\"\n"
   "ctx.font=\"10px sans-serif\"\n"
   "ctx.textAlign=\"center\"\n"
   "ht=(c.height-10)/4\n"
   "\n"
   "    dots=[]\n"
   "    date = new Date()\n"
   "ctx.lineWidth=10\n"
   "draw_scale(hours,c.width,ht,ht+3,0,date.getHours())\n"
   "ctx.lineWidth=7\n"
   "draw_scale(days,c.width,ht,ht*2+3,1,date.getDate()-1)\n"
   "ctx.lineWidth=15\n"
   "draw_scale(months,c.width,ht,ht*3+3,1,date.getMonth())\n"
   "\n"
   "// request mousemove events\n"
   "graph.mousemove(function(e){handleMouseMove(e);})\n"
   "\n"
   "// show tooltip when mouse hovers over dot\n"
   "function handleMouseMove(e){\n"
   "rect=c.getBoundingClientRect()\n"
   "mouseX=e.clientX-rect.x\n"
   "mouseY=e.clientY-rect.y\n"
   "var hit = false\n"
   "for(i=0;i<dots.length;i++){\n"
   "dot = dots[i]\n"
   "if(mouseX>=dot.x && mouseX<=dot.x2 && mouseY>=dot.y && mouseY<=dot.y2){\n"
   "tipCtx.fillStyle=\"#FFA\"\n"
   "tipCtx.fillRect(0,0,tipCanvas.width,tipCanvas.height)\n"
   "tipCtx.lineWidth = 2\n"
   "tipCtx.fillStyle = \"#000000\"\n"
   "tipCtx.strokeStyle = '#333'\n"
   "tipCtx.font = 'bold italic 10pt sans-serif'\n"
   "tipCtx.textAlign = \"left\"\n"
   "tipCtx.fillText( dot.tip, 4, 15)\n"
   "tipCtx.fillText( dot.tip2+' Wh', 4, 29)\n"
   "tipCtx.fillText( '$'+dot.tip3, 4, 44)\n"
   "hit = true\n"
   "popup = document.getElementById(\"popup\")\n"
   "popup.style.top =(dot.y+rect.y+window.pageYOffset) + \"px\"\n"
   "popup.style.left=(dot.x+rect.x-60) + \"px\"\n"
   "}\n"
   "}\n"
   "if (!hit) { popup.style.left = \"-200px\" }\n"
   "}\n"
   "\n"
   "function getMousePos(cDom, mEv){\n"
   "rect = cDom.getBoundingClientRect();\n"
   "return{\n"
   " x: mEv.clientX-rect.left,\n"
   " y: mEv.clientY-rect.top\n"
   "}\n"
   "}\n"
   "\n"
   "}catch(err){console.log(err)}\n"
   "})\n"
   "\n"
   "function draw_scale(arr,w,h,o,p,hi)\n"
   "{\n"
   "max=0\n"
   "for(i=0;i<arr.length;i++)\n"
   "if(arr[i][1]>max) max=arr[i][1]\n"
   "for(i=0;i<arr.length;i++)\n"
   "{\n"
   "x=i*(w/arr.length)+8\n"
   "ctx.strokeStyle=((arr[i][1]==max)?\"#F55\":\"#55F\")\n"
   "    bh=arr[i][1]*(h-20)/max\n"
   "    y=(o+h-20)-bh\n"
   "ctx.beginPath()\n"
   "    ctx.moveTo(x,o+h-20)\n"
   "    ctx.lineTo(x,y)\n"
   "ctx.stroke()\n"
   "ctx.strokeStyle=\"#FFF\"\n"
   "ctx.fillText(i+p,x,o+h-7)\n"
   "if(i==hi)\n"
   "{\n"
   "lw=ctx.lineWidth\n"
   "ctx.strokeStyle=\"#000\"\n"
   "ctx.lineWidth=1\n"
   "ctx.beginPath()\n"
   "    ctx.moveTo(x+lw+1,o+h-20)\n"
   "    ctx.lineTo(x+lw+1,o)\n"
   "ctx.stroke()\n"
   "ctx.lineWidth=lw\n"
   "}\n"
   "if(bh<16) bh=16\n"
   "if(arr[i][0])\n"
   "  dots.push({\n"
   "x: x,\n"
   "y: y,\n"
   "y2: y+bh,\n"
   "x2: x+ctx.lineWidth,\n"
   "tip: t2hms(arr[i][0]),\n"
   "tip2: arr[i][1],\n"
   "tip3: ((arr[i][1])*(+a.ppkw.value/1000)).toFixed(3)\n"
   "})\n"
   "}\n"
   "}\n"
   "</script>\n"
   "<style type=\"text/css\">\n"
   "#popup {\n"
   "  position: absolute;\n"
   "  top: 150px;\n"
   "  left: -150px;\n"
   "  z-index: 10;\n"
   "  border-style: solid;\n"
   "  border-width: 1px;\n"
   "}\n"
   "</style>\n"
   "</head>\n"
   "<body bgcolor=\"silver\">\n"
   "<table align=\"right\" width=\"450\">\n"
   "<tr><td>Light</td><td>LED1 &nbsp LED2</td><td><input id=\"nm\" type=text size=8 onchange=\"changeNm();\"></td><td><div id=\"time\"></div></td></tr>\n"
   "<tr><td><input name=\"RLY\" value=\"OFF\" type='button' onclick=\"{manual()}\"></td>\n"
   "<td><input name=\"LED1\" value=\"OFF\" type='button' onclick=\"{led1()}\">&nbsp <input name=\"LED2\" value=\"OFF\" type='button' onclick=\"{led2()}\"></td>\n"
   "<td colspan=2><label for=\"CH\">Rpt</label><input name=\"CH\" value=\"OFF\" type='button' onclick=\"{togCall()}\"> <label for=\"auto\"> Auto Off </label><input name=\"auto\" type=\"text\" value=\"0\" size=\"4\"> &nbsp <label for=\"TZ\">TZ </label><input name=\"TZ\" type=text size=1 value='-5'></td></tr>\n"
   "<tr>\n"
   "<td colspan=4>Start <input name=\"PF\" value=\"LAST\" type='button' onclick=\"{pf()}\"> &nbsp;<input name=\"LVL\" value=\"99\" type='text' size=\"2\"> &nbsp <input type=\"range\" id=\"level\" name=\"level\" min=1 max=200 onchange=\"slide();\">\n"
   "&nbsp; <input name=\"dly\" type=\"text\" value=\"0\" size=\"4\"><input value=\"Delay On\" type='button' onclick=\"{setDelay()}\"></td></tr>\n"
   "\n"
   "<tr><td>Name &nbsp </td><td colspan=3><div id=\"header\"><small>En Su Mo Tu We Th Fr Sa On Off</small> Time Duration Level</div></td></tr>\n"
   "</table>\n"
   "<table align=\"right\" style=\"font-size:small\" id=\"list\"></table>\n"
   "<table width=\"450\" align=\"right\" style=\"font-size:small\" id=\"dev\"><tr><td align=\"center\">&#8195; Name &#8195; &#8195; &nbsp; &nbsp; IP Address &#8195; &nbsp; Mode Dim Mt Fo Delay &nbsp; </td></tr></table>\n"
   "\n"
   "<table align=\"right\" width=450>\n"
   "<tr><td>PPKWH<input name=\"ppkw\" type=text size=3></td><td><div id=\"v\"></div></td><td>Watts<input name=watts type=text size=1>WH<input id=\"wh\" name=\"wh\" type=button size=8 onclick=\"clearWh();\"> $<input id=\"cost\" name=\"cost\" type=button size=8 onclick=\"clearWh();\"></td></tr>\n"
   "<tr><td><div id=\"rssi\">0db</div></td><td colspan=2><div id=\"ts\">1:00:00</div></td></tr>\n"
   "<tr><td id=\"MOT\">Motion<input name=\"mtime\" type=text size=4 style=\"width: 60px\"></td><td colspan=2>\n"
   " <input value='Save' type=button onclick=\"setData();\"> &nbsp; <input value='Restart' type=button onclick=\"setVar('reset',0);\"> &nbsp; <input id=\"myKey\" name=\"key\" type=text size=50 placeholder=\"password\" style=\"width: 128px\">\n"
   "<input type=\"button\" value=\"Set\" onClick=\"{localStorage.setItem('key', key = document.all.myKey.value)}\"></td></tr>\n"
   "</table>\n"
   "<table align=\"right\" width=450>\n"
   "<tr><td>\n"
   "<div id=\"wrapper\">\n"
   "<canvas id=\"chart\" width=\"444\" height=\"400\"></canvas>\n"
   "<div id=\"popup\"><canvas id=\"tip\" width=70 height=58></canvas></div>\n"
   "</td></tr>\n"
   "</table></div></body>\n"
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
