const char page1[] PROGMEM = R"rawliteral(
<!DOCTYPE html>
<html lang="en">
<head>
<meta name="viewport" content="width=device-width, initial-scale=1"/>
<title>WiFi Dimmer</title>
<style>
div,table{border-radius: 3px;box-shadow: 2px 2px 12px #000000;
background: rgb(94,94,94);
background: linear-gradient(0deg, rgba(94,94,94,1) 0%, rgba(160,160,160,1) 100%);
background-clip: padding-box;}
input{border-radius: 5px;box-shadow: 2px 2px 12px #000000;
background: rgb(160,160,160);
background: linear-gradient(0deg, rgba(160,160,160,1) 0%, rgba(239,239,239,1) 100%);
background-clip: padding-box;}
body{width:470px;display:block;margin-left:auto;margin-right:auto;text-align:right;font-family: Arial, Helvetica, sans-serif;}
</style>
<script src="http://ajax.googleapis.com/ajax/libs/jquery/1.6.1/jquery.min.js" type="text/javascript" charset="utf-8"></script>
<script type="text/javascript">
a=document.all
idx=0
nms=['OFF','ON ','LNK','REV']
dys=[['Sun'],['Mon'],['Tue'],['Wed'],['Thu'],['Fri'],['Sat']]
maxW=1
wArr=[]
wAdr=0
$(document).ready(function(){
  key=localStorage.getItem('key')
  if(key!=null) document.getElementById('myKey').value=key
  for(j=0;j<60*60;j++) wArr[j]=0
  openSocket()
})

function openSocket(){
ws=new WebSocket("ws://"+window.location.host+"/ws")
//ws=new WebSocket("ws://192.168.31.57/ws")
ws.onopen=function(evt){}
ws.onclose=function(evt){alert("Connection closed.");}
ws.onmessage=function(evt){
console.log(evt.data)
 lines=evt.data.split(';')
 event=lines[0]
 data=lines[1]
 d=JSON.parse(data)
 if(event=='state'){
  d2=new Date(d.t*1000)
  a.time.innerHTML=dys[d2.getDay()]+' '+d2.toLocaleTimeString()+' T:'+t2hms(d.tr)
  a.RLY.value=nms[d.on]
  a.RLY.setAttribute('style',d.on?'color:red':'')
  a.LED1.value=nms[d.l1]
  a.LED1.setAttribute('style',d.l1?'color:blue':'')
  a.LED2.value=nms[d.l2]
  a.LED2.setAttribute('style',d.l2?'color:blue':'')
  a.LVL.value=d.lvl
  a.level.value=d.lvl
  a.ts.innerHTML=t2hms(d.ts)+' &nbsp; Since ' + (new Date(d.st*1000)).toLocaleString()
  if(d.lvl==0)
  {
   a.LVL.style.visibility='hidden'
   a.level.style.visibility='hidden'
   a.header.innerHTML=' &nbsp Name &nbsp &nbsp &nbsp &nbsp En Su Mo Tu We Th Fr Sa On Off &nbsp Time &nbsp Duration &nbsp; &nbsp; &nbsp; &nbsp; &nbsp; '
  }
  a.MOT.setAttribute('bgcolor',d.mo?'red':'')
  if(d.sn) document.getElementById('r'+(d.sn-1)).setAttribute('bgcolor','red')
 }
 else if(event=='energy')
 {
  d2=new Date(d.t*1000)
  a.time.innerHTML=dys[d2.getDay()]+' '+d2.toLocaleTimeString()+' T:'+t2hms(d.tr)
  rs=d.rssi+'dB'
  if(d.v) {rs+=' &nbsp;&nbsp;'+d.v.toFixed(2)+'V';a.watts.value=d.p}
  a.rssi.innerHTML=rs
  a.v.innerHTML='&nbsp;&nbsp;'+d.c+'A'
  a.wh.value=d.wh.toFixed(2)
  a.cost.value=((+d.wh)*(+a.ppkw.value/1000)).toFixed(3)
  draw_point(d.p)
 }
 else if(event=='setup')
 {
  fillData(d)
 }
 else if(event=='hist')
 {
   days=d.days
   hours=d.hours
   months=d.months
   drawstuff()
 }
 else if(event=="watts")
 {
   for(i=0;i<d.watts.length;i++)
   {
     len=d.watts[i][0]
     if(len>0)
     {
       for(j=0;j<len;j++)
       { wt=d.watts[i][1+j]
         wArr[wAdr++]=wt
         if(wt>maxW) maxW=wt
       }
     }
     else
     {
       len=-len
       wt=d.watts[i][1]
       for(j=0;j<len;j++) wArr[wAdr++]=wt
       if(wt>maxW) maxW=wt
     }
   }
 }
 else if(event=='update')
 {
  switch(d.type)
  {
    case 'hour':
    hours[d.e][0]=d.sec
    hours[d.e][1]=d.p
    break
    case 'day':
    days[d.e][0]=d.sec
    days[d.e][1]=d.p
    break
  }
  update_bars()
 }
 else if(event=='dev')
 {
   updateDev(+d.dev,+d.on,+d.level)
 }
 else if(event=='alert'){alert(data)}
}
}
function setVar(varName, value)
{
 ws.send('cmd;{"key":"'+a.myKey.value+'","'+varName+'":'+value+'}')
}
function manual()
{
on=(a.RLY.value=='OFF')
setVar('pwr',on)
a.RLY.value=on?'ON ':'OFF'
a.RLY.setAttribute('style',on?'color:red':'')
}

function pf()
{
if(a.PF.value=='LAST') l=1
else if(a.PF.value=='OFF ') l=2
else l=0
setVar('powerup',l)
pus=['LAST','OFF ',' ON ']
a.PF.value=pus[l]
}

function led1()
{
if(a.LED1.value=='OFF') l=1
else if(a.LED1.value=='ON ') l=2
else if(a.LED1.value=='LNK') l=3
else l=0
setVar('led1',l)
a.LED1.value=nms[l]
a.LED1.setAttribute('style',l?'color:blue':'')
}
function led2()
{
if(a.LED2.value=='OFF') l=1
else if(a.LED2.value=='ON ') l=2
else if(a.LED2.value=='LNK') l=3
else l=0
setVar('led2',l)
a.LED2.value=nms[l]
a.LED2.setAttribute('style',l?'color:blue':'')
}

function t2hms(t)
{
  s=t%60
  t=Math.floor(t/60)
  if(t==0) return s
  if(s<10) s='0'+s
  m=t%60
  t=Math.floor(t/60)
  if(t==0) return m+':'+s
  if(m<10) m='0'+m
  h=t%24
  t=Math.floor(t/24)
  if(t==0) return h+':'+m+':'+s
  return t+'d '+h+':'+m+':'+s
}

function hms2t(s)
{
  ar=s.split(':')
  t=0
  for(i=0;i<ar.length;i++){t*=60;t+=+ar[i]}
  return t
}

function setData(ent)
{
 id=ent.id.substr(0,1)
 idx=ent.id.substr(1)
 setVar('I',idx)
 switch(id)
 {
  case 'N':
    setVar('N',setup.sched[idx][0]=document.getElementById('N'+idx).value)
    break
  case 'W':
    wks=document.getElementsByName('W'+idx)
    w=0
    for(wd=0;wd<10;wd++) if(wks[wd].checked) w|=(1<<wd)
    setVar('W',w)
      setup.sched[idx][3]=w
    break
  case 'S':
    setVar('S',setup.sched[idx][1]=hms2t(document.getElementById('S'+it).value))
    break
  case 'T':
    setVar('T',setup.sched[idx][2]=hms2t(document.getElementById('T'+it).value))
    break
  case 'L':
    setVar('L',setup.sched[it][4]=document.getElementById('L'+it).value)
    break
  }
  if(setup.sched[setup.sched.length-1][0]!='')
  {
   setup.sched.push(['',60,60,255])
   AddEntry(setup.sched[setup.sched.length-1])
  }
}

function togCall()
{
  on=(a.CH.value=='OFF')?1:0
  a.CH.value=nms[on]
  a.CH.setAttribute('style',on?'color:red':'')
  if(on) setVar('hostip',80)
  else setVar('ch',0)
}

function slide()
{
 lvl=a.level.value
 a.LVL.value=lvl
 setVar('level',lvl)
}

function setDelay()
{
  setVar('dlyon',hms2t(a.dly.value))
}

function setAuto()
{
  setVar('auto',hms2t(a.auto.value))
}

function setMTime()
{
  setVar('MOT',hms2t(a.mtime.value))
}

function clearWh()
{
  setVar('rt',0)
  a.cost.value='0.000'
  a.wh.value='0.00'
}

function changeNm()
{
  setVar('devname','"'+a.nm.value+'"')
}

function fillData(set)
{
  a.TZ.value=set.tz
  a.nm.value=set.name
  document.title=set.name
  a.mtime.value=t2hms(set.mot)
  a.auto.value=t2hms(set.auto)
  a.CH.value=set.ch?' ON ':'OFF'
  pus=['LAST','OFF ',' ON ']
  a.PF.value=pus[set.pu]
  a.CH.setAttribute('style',set.ch?'color:red':'')
  a.watts.value=set.watt
  a.ppkw.value=set.ppkw/1000
  for(it=0;it<set.sched.length;it++)
    AddEntry(set.sched[it])
  idx=0
  for(d=0;d<set.dev.length;d++)
    AddDev(set.dev[d])

  tr=document.createElement("tr")
  tbody=document.createElement("tbodypad")
  tbody.appendChild(tr)
  a.dev.appendChild(tbody)
}

function AddEntry(arr)
{
  td=document.createElement("td")

  inp=document.createElement("input")
  inp.id='N'+idx
  inp.type='text'
  inp.size=8
  inp.value=arr[0]
  inp.onchange=function(){setData(this);}
  td.appendChild(inp)

  for(i=0;i<10;i++){
  inp=document.createElement("input")
  inp.type='checkbox'
  inp.id='W'+idx
  inp.name='W'+idx
  inp.checked=(arr[3]&(1<<i))
  inp.onclick=function(){setData(this);}
  td.appendChild(inp)
  }

  inp=document.createElement("input")
  inp.id='S'+idx
  inp.type='text'
  inp.value=t2hms(arr[1])
  inp.size=3
  inp.onchange=function(){setData(this);}
  td.appendChild(inp)

  inp=document.createElement("input")
  inp.id='T'+idx
  inp.type='text'
  t=t2hms(arr[2])
  inp.value=(t)?t:''
  inp.size=2
  inp.onchange=function(){setData(this);}
  td.appendChild(inp)

  inp=document.createElement("input")
  inp.id='L'+idx
  inp.type='text'
  inp.value=arr[4]?arr[4]:''
  inp.size=1
  inp.onchange=function(){setData(this);}
  td.appendChild(inp)

  tr=document.createElement("tr")
  tr.id='r'+idx
  tr.appendChild(td)
  tbody=document.createElement("tbody")
  tbody.id='tbody'+idx
  tbody.appendChild(tr)
  a.list.appendChild(tbody)
  idx++
}

function changeDev(dev)
{
 idx=dev.id.split('-')
 val=document.getElementById(dev.id).value
 switch(idx[0])
 {
  case 'DE':
    s=document.getElementById('IP-'+idx[1]).innerHTML
    window.location.href='http://'+s
    break
  case 'MD':
    mode=0
      if(val=='------'){ document.getElementById(dev.id).value='LNK'; mode=1}
      else if(val=='LNK'){document.getElementById(dev.id).value='REV'; mode=2}
    else document.getElementById(dev.id).value='------'
    setVar('DEV',idx[1])
    setVar('MODE',mode)
    break
  case 'CK':
    cb=document.getElementsByName('CK-'+idx[1])
    setVar('DEV',idx[1])
    setVar('DIM',(cb[0].checked)?1:0)
    setVar('OP',(cb[1].checked)?1:0)
    break
  case 'DLY':
    setVar('DEV',idx[1])
    setVar('DLY',hms2t(val))
    break
  case 'ON':
    setVar('DEV',idx[1])
    v=(val=='OFF')?1:0
    setVar('DON',v)
    btn=document.getElementById(dev.id)
    btn.value=nms[v]
  btn.setAttribute('style',v?'color:red':'')
    break
 }
}

// "name","192.168.0.122",mode,flags,dly
function AddDev(arr)
{
  td=document.createElement("td")
  td.align="center"

  inp=document.createElement("input")
  inp.id='DE-'+idx
  inp.type='button'
  inp.style.width='105px'
  inp.value=arr[0]
  inp.onclick=function(){changeDev(this); }
  td.appendChild(inp)

  inp=document.createElement("button")
  inp.id='IP-'+idx
  inp.border=1
  inp.style.width='105px'
  inp.type='text'
  inp.innerHTML=arr[1]
  td.appendChild(inp)

  inp=document.createElement("input")
  inp.id='MD-'+idx
  inp.type='button'
  nams=['------','LNK','REV']
  inp.value=nams[arr[2]]
  inp.onclick=function(){changeDev(this); }
  td.appendChild(inp)

  for(i=0;i<2;i++)
  {
  inp=document.createElement("input")
  inp.id='CK-'+idx
  inp.name='CK-'+idx
  inp.type='checkbox'
  inp.checked=(arr[3]&(1<<i))?1:0
  inp.onclick=function(){changeDev(this); }
  td.appendChild(inp)
  }
  inp=document.createElement("input")
  inp.id='DLY-'+idx
  inp.type='text'
  inp.size=3
  inp.value=t2hms(arr[4])
  inp.onchange=function(){changeDev(this); }
  td.appendChild(inp)

  inp=document.createElement("input")
  inp.id='ON-'+idx
  inp.type='button'
  inp.style.width='40px'
  inp.value=nms[arr[5]]
  inp.setAttribute('style',arr[5]?'color:red':'')
  inp.onclick=function(){changeDev(this); }
  td.appendChild(inp)

  tr=document.createElement("tr")
  tr.id='dv'+idx
  tr.appendChild(td)
  tbody=document.createElement("tbody")
  tbody.id='tbody'+idx
  tbody.appendChild(tr)

  a.dev.appendChild(tbody)
  idx++
}

function updateDev(dev,on,level)
{
  btn=document.getElementById('ON-'+dev)
  btn.value=nms[on]
  btn.setAttribute('style',on?'color:red':'')
}

function draw_point(wt){
try {
  var c=document.getElementById('chart')
  ctx=c.getContext("2d")
  ctx.fillStyle="#225"
  ht=(c.height/4)-2
  ctx.fillRect(0,0,c.width,ht+2)
  date=new Date()
  if(wt>maxW) maxW=wt
  ctx.textAlign="left"
  ctx.fillStyle="#888"
  ctx.fillText(maxW.toFixed(2),1,8)
  ctx.fillText((maxW/2).toFixed(2),1,ht/2+4)
  ctx.strokeStyle="#888"
  ctx.lineWidth=0.4
  ctx.beginPath()
  ctx.moveTo(30,ht/2)
  ctx.lineTo(c.width-3,ht/2)
  ctx.stroke()
  ctx.textAlign="right"
  cp=date.getMinutes()*60+date.getSeconds()
  if(wArr.length>=cp)
  {
    if(cp&&wArr[cp-1]==0)
      wArr[cp-1]=wt
    wArr[cp]=wt
  }
  ctx.strokeStyle="#EEE"
  ctx.lineWidth=0.4
  ctx.beginPath()
  x=cp*c.width/3600
  ctx.moveTo(x,0)
  ctx.lineTo(x,ht)
  ctx.stroke()
  ctx.strokeStyle="#F55"
  ctx.fillStyle="#FFF"
  ctx.lineWidth=1.5
  mvd=0
  t=0

  for(i=0;i<3600;i++)
  {
    if(wArr[i]!=undefined)
    {
      x=i*c.width/3600
      y=wArr[i]*(ht-2)/maxW
      t+=wArr[i]
      if(!mvd){ctx.beginPath();ctx.moveTo(x,ht-y)}
      else ctx.lineTo(x,ht-y)
        if(i==cp){mvd=false;ctx.stroke()}
      else mvd=true
    }

    if((i%600)==0) ctx.fillText(i/60,i*c.width/3600,ht)
  }
  ctx.stroke()
  ctx.fillText((t/1000).toFixed(2)+' Wh',c.width-1,8)
  ctx.fillText('$'+(t/1000*(+a.ppkw.value/1000)).toFixed(3),c.width-1,18)
}catch(err){}
}

function update_bars()
{
  var c=document.getElementById('chart')
  ctx=c.getContext("2d")
  ht=c.height/4
  ctx.fillStyle="#FFF"
  ctx.font="10px sans-serif"

    dots=[]
    date = new Date()
  ctx.lineWidth=10
  draw_scale(hours,c.width,ht,ht+3,0,date.getHours())
  ctx.lineWidth=7
  draw_scale(days,c.width,ht,ht*2+3,1,date.getDate()-1)
  ctx.lineWidth=15
  draw_scale(months,c.width,ht,ht*3+3,1,date.getMonth())
}

function drawstuff(){
try {
  graph = $('#chart')
  var c=document.getElementById('chart')
  rect=c.getBoundingClientRect()
  canvasX=rect.x
  canvasY=rect.y

    tipCanvas=document.getElementById("tip")
    tipCtx=tipCanvas.getContext("2d")
    tipDiv=document.getElementById("popup")

  ctx=c.getContext("2d")
  ht=c.height/4
  ctx.fillStyle="#FFF"
  ctx.font="10px sans-serif"

    dots=[]
    date = new Date()
  ctx.lineWidth=10
  draw_scale(hours,c.width,ht,ht+3,0,date.getHours())
  ctx.lineWidth=7
  draw_scale(days,c.width,ht,ht*2+3,1,date.getDate()-1)
  ctx.lineWidth=15
  draw_scale(months,c.width,ht,ht*3+3,1,date.getMonth())

  // request mousemove events
  graph.mousemove(function(e){handleMouseMove(e);})

  // show tooltip when mouse hovers over dot
  function handleMouseMove(e){
    rect=c.getBoundingClientRect()
    mouseX=e.clientX-rect.x
    mouseY=e.clientY-rect.y
    var hit = false
    for(i=0;i<dots.length;i++){
      dot = dots[i]
      if(mouseX>=dot.x && mouseX<=dot.x2 && mouseY>=dot.y && mouseY<=dot.y2){
        tipCtx.clearRect(0,0,tipCanvas.width,tipCanvas.height)
        tipCtx.fillStyle="#FFA"
        tipCtx.lineWidth = 2
        tipCtx.fillStyle = "#000000"
        tipCtx.strokeStyle = '#333'
        tipCtx.font = 'bold italic 10pt sans-serif'
        tipCtx.textAlign = "left"
        tipCtx.fillText(t2hms(dot.tip),4,15)
        if(+dot.tip2>1000)
         txt=(dot.tip2/1000).toFixed(1)+' KWh'
        else
         txt=dot.tip2+' Wh'
        tipCtx.fillText(txt, 4, 29)
        tipCtx.fillText('$'+(dot.tip2*(+a.ppkw.value/1000)).toFixed(3),4,44)
        hit=true
        popup=document.getElementById("popup")
        popup.style.top=(dot.y+rect.y+window.pageYOffset)+"px"
        popup.style.left=(dot.x+rect.x-60)+"px"
      }
    }
    if(!hit) popup.style.left="-200px"
  }

  function getMousePos(cDom, mEv){
    rect = cDom.getBoundingClientRect();
    return{
     x: mEv.clientX-rect.left,
     y: mEv.clientY-rect.top
    }
  }
}catch(err){}
}

function draw_scale(arr,w,h,o,p,hi)
{
  ctx.fillStyle="#336"
  ctx.fillRect(0,o,w,h-3)
  ctx.fillStyle="#FFF"
  max=0
  tot=0
  for(i=0;i<arr.length;i++)
  {
    if(arr[i][1]>max) max=arr[i][1]
    tot+=arr[i][1]
  }
  ctx.textAlign="center"
  for(i=0;i<arr.length;i++)
  {
    x=i*(w/arr.length)+8
    ctx.strokeStyle="#55F"
      bh=arr[i][1]*(h-20)/max
      y=(o+h-20)-bh
    ctx.beginPath()
      ctx.moveTo(x,o+h-20)
      ctx.lineTo(x,y)
    ctx.stroke()
    ctx.strokeStyle="#FFF"
    ctx.fillText(i+p,x,o+h-7)
    if(i==hi)
    {
      lw=ctx.lineWidth
      ctx.strokeStyle="#000"
      ctx.lineWidth=1
      ctx.beginPath()
        ctx.moveTo(x+lw+1,o+h-20)
        ctx.lineTo(x+lw+1,o)
      ctx.stroke()
      ctx.lineWidth=lw
    }
    if(bh<16) bh=16
    if(arr[i][0])
      dots.push({
      x: x-(ctx.lineWidth/2),
      y: y,
      y2: y+bh,
      x2: x+ctx.lineWidth,
      tip: arr[i][0],
      tip2: arr[i][1]
    })
  }
  ctx.textAlign="right"
  if(tot>1000)
   txt=(tot/1000).toFixed(1)+' KWh'
  else
   txt=tot.toFixed(1)+' Wh'
  ctx.fillText(txt,w-1,o+10)
  ctx.fillText('$'+(tot*(+a.ppkw.value/1000)).toFixed(3) ,w-1,o+21)
}
</script>
<style type="text/css">
#popup {
  position: absolute;
  top: 150px;
  left: -150px;
  z-index: 10;
  border-style: solid;
  border-width: 1px;
}
</style>
</head>
<body bgcolor="silver">
<table align="right" width=450>
<tr><td>Power</td><td>LED1 &nbsp; LED2</td><td><input id="nm" type=text size=8 onchange="changeNm();"></td><td><div id="time"></div></td></tr>
<tr><td><input name="RLY" value="OFF" type='button' onclick="{manual()}"></td>
<td><input name="LED1" value="OFF" type='button' onclick="{led1()}">&nbsp; <input name="LED2" value="OFF" type='button' onclick="{led2()}"></td>
<td colspan=2><label for="CH">Rpt</label><input name="CH" value="OFF" type='button' onclick="{togCall()}"> Auto Off <input name="auto" type="text" value="0" size="4" onchange="setAuto();">
 &nbsp; TZ<input name="TZ" type=text size=1 value='-5' onchange="setVar('TZ', this.value);"></td></tr>
<tr>
<td colspan=4>Start <input name="PF" value="LAST" type='button' onclick="{pf()}"> &nbsp;<input name="LVL" value="99" type='text' size="2"> &nbsp; <input type="range" id="level" name="level" min=1 max=200 onchange="slide();">
&nbsp; <input name="dly" type="text" value="0" size="4"><input value="Delay On" type='button' onclick="{setDelay()}"></td></tr>
</table>
<table align="right" style="font-size:small" id="list">
<tr><td id="header" align="center"> &nbsp; Name &nbsp; &nbsp; &nbsp; &nbsp; En Su Mo Tu We Th Fr Sa On Off &nbsp; Time &nbsp; Duration Level</td></tr>
</table>
<table width=450 align="right" style="font-size:small" id="dev"><tr><td align="center">&#8195; Name &#8195; &#8195; &nbsp; &nbsp; IP Address &#8195; &nbsp; Mode Dim Mt Delay &nbsp; ON </td></tr></table>
<table align="right" width=450>
<tr><td>PPKWH<input name="ppkw" type=text size=3 onchange="setVar('ppkw',+this.value*1000);"></td><td>
<div id="v" style="width:50px"></div></td><td>Watts<input name=watts type=text size=1 onchange="setVar('watts',this.value);">WH<input id="wh" name="wh" type=button size=8 onclick="clearWh();"> $<input id="cost" name="cost" type=button size=8 onclick="clearWh();"></td></tr>
<tr><td><div id="rssi">0db</div></td><td colspan=2><div id="ts">1:00:00</div></td></tr>
<tr><td id="MOT">Motion<input name="mtime" type=text size=4 style="width: 60px" onchange="setMTime();"></td><td colspan=2>
 <input value='Restart' type=button onclick="setVar('reset',0);"> &nbsp; 
 <input id="myKey" name="key" type=text size=50 placeholder="password" style="width: 128px" onChange="{localStorage.setItem('key', key = document.all.myKey.value)}">
</td></tr>
</table>
<table align="right" width=450>
<tr><td>
<div id="wrapper">
<canvas id="chart" width="444" height="400"></canvas>
<div id="popup"><canvas id="tip" width=70 height=58></canvas></div>
</div></td></tr>
</table></body>
</html>
)rawliteral";

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
