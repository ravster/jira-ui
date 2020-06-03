import requests
import json

file = open("creds", "r")
lines = file.readlines()

basic_auth = lines[0].strip().split(":")
subdomain = lines[1].strip()

current_issue = ""

def did_not_understand_command():
    print("Sorry, we didn't understand that command.")

def goto_issue(id):
    uri = f'https://{subdomain}.atlassian.net/rest/api/latest/issue/{id}'
    r = requests.get(uri, auth=(basic_auth[0], basic_auth[1]))
    global current_issue
    current_issue = r.json()

def description():
    print(current_issue['fields']['description'])

def summary():
    print(current_issue['fields']['summary'])

def comments():
    for comment in current_issue['fields']['comment']['comments']:
        author = comment['author']['displayName']
        body = comment['body']
        print(f'{author}: {body}')
        print('-----')

def print_all():
    print(current_issue)

def my_issues():
    body = {'jql': "assignee = 'Ravi' and status != Done order by updated asc"}
    uri = f'https://{subdomain}.atlassian.net/rest/api/2/search'
    r = requests.get(uri, auth=(basic_auth[0], basic_auth[1]), params=body)
    r1 = r.json()
    for issue in r1['issues']:
        print(f"{issue['key']}: {issue['fields']['summary']}")

def read_command():
    c = input("> ")
    if c.startswith("g "):
        goto_issue(c.split()[1])
    elif c.startswith("d"):
        description()
    elif c.startswith("s"):
        summary()
    elif c.startswith('c'):
        comments()
    elif c.startswith('p'):
        print_all()
    elif c.startswith('mine'):
        my_issues()
    else:
        did_not_understand_command()

while True:
    read_command()
