from pathlib import Path
from playwright.sync_api import sync_playwright
root=Path(__file__).resolve().parents[1]
with sync_playwright() as p:
 b=p.chromium.launch(headless=True,executable_path='/usr/bin/chromium',args=['--no-sandbox','--disable-gpu'])
 page=b.new_page(viewport={'width':800,'height':1050})
 errors=[];page.on('pageerror',lambda e:errors.append(str(e)))
 page.set_content((root/'web/STAR_VOYAGER_STANDALONE.html').read_text(encoding='utf-8'),wait_until='domcontentloaded')
 page.wait_for_timeout(250)
 assert page.evaluate('window.STAR_TUTORIAL_OPEN')
 r=page.locator('#tutorialOverlay').bounding_box()
 page.mouse.click(r['x']+r['width']*.5,r['y']+r['height']*.84)
 page.wait_for_timeout(80)
 assert not page.evaluate('window.STAR_TUTORIAL_OPEN')
 vals=page.evaluate('''()=>[-1,-.75,-.5,-.25,0,.25,.5,.75,1].map(t=>{let a=Math.PI/2-t*2*Math.PI/3;let x=180*Math.cos(a),y=180*Math.sin(a);return {t,v:ringThrottle(x,y),valid:throttleBand(x,y)}})''')
 for x in vals:
  assert x['valid'] and abs(x['t']-x['v'])<.1,x
 assert not page.evaluate('throttleBand(0,-190)')
 cv=page.locator('#screen').bounding_box();cpx=cv['x']+cv['width']/2;cpy=cv['y']+cv['height']/2
 scale=cv['width']/466
 def pt(t):
  from math import pi,cos,sin
  a=pi/2-t*2*pi/3
  return cpx+180*scale*cos(a),cpy+180*scale*sin(a)
 # Starting from the neutral and swiping towards both ends must reach full output.
 page.mouse.move(*pt(0));page.mouse.down()
 for i in range(1,11):page.mouse.move(*pt(i/10))
 page.mouse.up()
 assert page.evaluate('throttle')>.98,page.evaluate('throttle')
 page.mouse.move(*pt(0));page.mouse.down()
 for i in range(1,11):page.mouse.move(*pt(-i/10))
 page.mouse.up()
 assert page.evaluate('throttle')<-.98,page.evaluate('throttle')
 page.keyboard.press('KeyQ');assert page.evaluate('warpMenu')
 page.keyboard.press('ArrowRight');assert page.evaluate('navIndex')==1
 page.keyboard.press('KeyQ');assert not page.evaluate('warpMenu')
 assert not errors,errors
 print('PASS R11 browser: tutorial START, 9 ring positions, full FWD/REV finger swipe, warp menu and no JS errors')
 b.close()
