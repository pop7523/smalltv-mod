"""Read-only public-endpoint probe. Availability is separate from image checks."""
import json
import urllib.request
import urllib.error
class NoRedirect(urllib.request.HTTPRedirectHandler):
    def redirect_request(self,*args,**kwargs):return None
url='http://api.open-meteo.com/v1/forecast?latitude=37.5665&longitude=126.9780&current=temperature_2m,relative_humidity_2m'
try:
    with urllib.request.build_opener(NoRedirect).open(url,timeout=12) as response:
        data=json.load(response)
        assert 'temperature_2m' in data['current'] and 'relative_humidity_2m' in data['current']
        print('Open-Meteo plain HTTP probe: PASS (200; current temperature and humidity)')
except Exception as error:
    print('Open-Meteo plain HTTP probe: UNVERIFIED:',str(error))
