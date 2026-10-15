"""Run the complete SDL reader against a deterministic local comic service."""
import http.server,json,os,pathlib,struct,subprocess,sys,tempfile,threading,shlex
pages=[]
# Portrait bitmap with an obvious page area for render validation.
w,h=400,800
pixels=bytes([90,135,190])*w*h
bmp=b'BM'+struct.pack('<IHHI',54+len(pixels),0,0,54)+struct.pack('<IiiHHIIiiII',40,w,h,1,24,0,len(pixels),2835,2835,0,0)+pixels
class Handler(http.server.BaseHTTPRequestHandler):
 def log_message(self,*args):pass
 def respond(self,value,mime='application/json'):
  b=json.dumps(value).encode() if mime=='application/json' else value
  self.send_response(200);self.send_header('Content-Type',mime);self.send_header('Content-Length',str(len(b)));self.end_headers();self.wfile.write(b)
 def do_POST(self):
  body=self.rfile.read(int(self.headers.get('Content-Length',0)))
  assert self.headers['X-API-Key']=='fixture-key'
  if '/channel/' in self.path:self.respond({'data':{'book_ids':['series-1']}})
  else:self.respond({'content':[{'id':'book-1','number':1,'media':{'pagesCount':3}},{'id':'book-2','number':2,'media':{'pagesCount':2}}],'last':True})
 def do_GET(self):
  assert self.headers['X-API-Key']=='fixture-key'
  if '/pages/' in self.path:pages.append(int(self.path.rsplit('/',1)[1]));self.respond(bmp,'image/bmp')
  elif self.path.endswith('/thumbnail'):self.respond(pathlib.Path('romfs/logo.png').read_bytes(),'image/png')
  else:self.respond({'id':'series-1','name':'Reader fixture','author':'Test author'})
server=http.server.ThreadingHTTPServer(('127.0.0.1',0),Handler)
threading.Thread(target=server.serve_forever,daemon=True).start()
with tempfile.TemporaryDirectory() as td:
 td=pathlib.Path(td);url=f'http://127.0.0.1:{server.server_port}'
 config=td/'client.json';config.write_text(json.dumps({'base':url,'gateway':url,'api_key':'fixture-key'}))
 subprocess.run(['g++','-shared','-fPIC','-std=c++17','tests/ui_input.cpp',*shlex.split(subprocess.check_output(['pkg-config','--cflags','sdl2'],text=True)),'-ldl','-o',str(td/'input.so')],check=True)
 state=td/'state.json';shot=td/'reader.bmp'
 env=dict(os.environ,SDL_VIDEODRIVER='dummy',LD_PRELOAD=str(td/'input.so'))
 subprocess.run([sys.argv[1],'--api',str(config),'--state',str(state),'--frames','1200','--screenshot',str(shot)],env=env,check=True,timeout=25)
 saved=json.loads(state.read_text());assert saved['history']['series-1']['page']==2
 assert saved['favorites']['series-1']['name']=='Reader fixture'
 assert pages==[1,2,3,2,3,2,2],pages
 assert saved["portrait"] is True
 im=shot.read_bytes();offset=struct.unpack_from('<I',im,10)[0]
 # SDL screenshot is 32-bit BMP; center of the image must contain the fixture color.
 assert bytes([90,135,190]) in im[offset:]
 width,height=struct.unpack_from('<ii',im,18)
 def pixel(x,y):
  row=height-1-y if height>0 else y
  return im[offset+row*width*4+x*4:offset+row*width*4+x*4+3]
 # In portrait mode the top toolbar rotates onto the physical right edge.
 assert pixel(1250,360)==bytes([43,27,20]),pixel(1250,360)
 assert pixel(640,360)==bytes([90,135,190]),pixel(640,360)
 print('Full reader UI passed: browse → chapter → page 1 → page 2 → portrait/landscape rotation → remapped page keys → back → resume; orientation/favorites persisted')
server.shutdown()
