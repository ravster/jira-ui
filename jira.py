import requests
import json

file = open("creds", "r")
lines = file.readlines()

basic_auth = lines[0].strip().split(":")
subdomain = lines[1].strip()

current_issue = ""

def did_not_understand_command():
    print("Sorry, we didn't understand that command.")

def summary():
    print(current_issue['fields']['status']['name'])
    print(current_issue['fields']['summary'])
    print(current_issue['fields']['description'])

def goto_issue(id):
    uri = f'https://{subdomain}.atlassian.net/rest/api/latest/issue/{id}'
    r = requests.get(uri, auth=(basic_auth[0], basic_auth[1]))
    global current_issue
    current_issue = r.json()
    global issue_id
    issue_id = id
    summary()

def comments():
    for comment in current_issue['fields']['comment']['comments']:
        author = comment['author']['displayName']
        body = comment['body']
        print(f'{author}: {body}')
        print('-----')

def print_all():
    print(json.dumps(current_issue, indent=2))

def my_issues():
    body = {'jql': "assignee = 'Ravi' and status != Done order by updated asc"}
    uri = f'https://{subdomain}.atlassian.net/rest/api/2/search'
    r = requests.get(uri, auth=(basic_auth[0], basic_auth[1]), params=body)
    r1 = r.json()
    for issue in r1['issues']:
        print(f"{issue['key']}: {issue['fields']['status']['name']}: {issue['fields']['summary']}")
    print("--- Newly updated issues are lower ---")

def make_comment():
    print("Write your comment and end with a '.' on it's own line.")
    a1 = []
    while True:
        a2 = input('|')
        if (len(a2) == 1 and a2[0] == '.'):
            break
        a1.append(a2)
    a3 = "\n".join(a1)

    uri = f'https://{subdomain}.atlassian.net/rest/api/2/issue/{issue_id}/comment'
    r = requests.post(uri, auth=(basic_auth[0], basic_auth[1]), json={'body': a3})
    r1 = r.json()
    print(r1)

def print_transitions():
    uri = f'https://{subdomain}.atlassian.net/rest/api/2/issue/{issue_id}/transitions'
    r = requests.get(uri, auth=(basic_auth[0], basic_auth[1]))
    r1 = r.json()
    for t in r1['transitions']:
        print(f"{t['id']}: {t['name']}")

def do_transition(id):
    uri = f'https://{subdomain}.atlassian.net/rest/api/2/issue/{issue_id}/transitions'
    r = requests.post(uri, auth=(basic_auth[0], basic_auth[1]), json={'transition': {'id': id}})
    print(r.status_code)

def read_command():
    c = input("> ")
    if c.startswith("g "):
        goto_issue(c.split()[1])
    elif c.startswith("s"):
        summary()
    elif c.startswith('c'):
        comments()
    elif c.startswith('pt'):
        print_transitions()
    elif c.startswith('p'):
        print_all()
    elif c.startswith('mine'):
        my_issues()
    elif c.startswith('m'):
        make_comment()
    elif c.startswith('t'):
        do_transition(c.split()[1])
    else:
        did_not_understand_command()

while True:
    read_command()
