import bs4 as bs
import requests

res = requests.get('https://en.wikipedia.org/wiki/List_of_ISO_639_language_codes')
soup = bs.BeautifulSoup(res.text, 'html.parser')

table = soup.find('table', {'class': 'wikitable sortable'})
tbody = table.find('tbody')
trs = tbody.find_all('tr')

for tr in trs:
    tds = tr.find_all('td')
    try:
        print(f"{"{"}\"{tds[0].text}\",\"{tds[1].text}\"{"}"},")
    except IndexError:
        pass