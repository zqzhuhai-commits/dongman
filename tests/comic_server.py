"""Protocol fixture: assert APK request shape, auth header and page base."""
import http.server,json,subprocess,sys,tempfile,threading
from urllib.parse import urlparse,parse_qs
class Handler(http.server.BaseHTTPRequestHandler):
 def log_message(self,*args): pass
 def send(self,value,mime='application/json'):
  b=json.dumps(value).encode() if mime=='application/json' else value
  self.send_response(200);self.send_header('Content-Type',mime);self.send_header('Content-Length',str(len(b)));self.end_headers();self.wfile.write(b)
 def do_POST(self):
  assert self.headers['X-API-Key']=='fixture-key'
  body=self.rfile.read(int(self.headers.get('Content-Length',0)))
  p=urlparse(self.path)
  if p.path=='/api/v1/channel/omo-all':
   assert json.loads(body)=={};self.send({'data':{'book_ids':['series-1','series-2','series-1']}})
  elif p.path=='/api/v1/books/list':
   assert json.loads(body)=={'condition':{'allOf':[{'seriesId':{'operator':'is','value':'series-1'}}]}}
   page=int(parse_qs(p.query)['page'][0]);assert parse_qs(p.query)['size']==['100']
   ns=range(100,0,-1) if page==0 else [101]
   self.send({'content':[{'id':f'book-{i}','number':str(i),'media':{'pagesCount':12}} for i in ns],'last':page==1})
  else: self.send_error(404)
 def do_GET(self):
  assert self.headers['X-API-Key']=='fixture-key'
  if self.path=='/api/v1/series/series-1':self.send({'id':'series-1','name':'Test comic','author':'Test'})
  elif self.path=='/api/v1/books/book-1/pages/1':self.send(b'image-fixture','image/png')
  else:self.send_error(404)
server=http.server.ThreadingHTTPServer(('127.0.0.1',0),Handler)
threading.Thread(target=server.serve_forever,daemon=True).start()
with tempfile.NamedTemporaryFile(mode='w',suffix='.json') as config:
 url=f'http://127.0.0.1:{server.server_port}'
 json.dump({'base':url,'gateway':url,'api_key':'fixture-key'},config);config.flush()
 subprocess.run([sys.argv[1],config.name],check=True)
server.shutdown()
