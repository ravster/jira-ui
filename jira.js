const readline = require("readline")
const axios = require("axios")

const file = require("fs").readFileSync("creds").toString()
const lines = file.split('\n')
const creds = lines[0].split(":")
const username = creds[0]
const password = creds[1]
const subdomain = lines[1]

var currentIssue

const rl = readline.createInterface({
  input: process.stdin,
  output: process.stdout
});

function goto_issue(id) {
  return new Promise((resolve, reject) => {
    axios.get(`https://${subdomain}.atlassian.net/rest/api/latest/issue/${id}`, {
      auth: {
	username: username,
	password: password
      }
    })
      .then(response => {
	console.log("Got issue")
	currentIssue = response.data
	resolve()
      })
      .catch(error => {
	console.log(error)
      })
  })
}

function print_comments() {
  currentIssue.fields.comment.comments.forEach((i) => {
    console.log(`${i.author.displayName}: ${i.body}`)
  })
}

function eval_command(command) {
  return new Promise((resolve, reject) => {
    switch(command.charAt(0)) {
    case 'g':
      goto_issue(command.slice(2)).then(() => resolve())
      break
    case 'd':
      console.log(currentIssue.fields.description)
      resolve()
      break
    case 'c':
      print_comments()
      resolve()
      break
    case 's':
      console.log(currentIssue.fields.summary)
      resolve()
      break
    default:
      console.log("You entered", command)
      resolve()
    }
  })
}

function get_command() {
  rl.question("> ", command => {
    eval_command(command).then(() => get_command())
  })
}

get_command()
