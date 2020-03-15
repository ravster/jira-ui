set cred_file [open creds]
set creds [gets $cred_file]
set subdomain [gets $cred_file]
close $cred_file

# Yes, I'm using globals.  Deal with it.
set id ""
set issue ""

# Usage: issue abc-234
proc issue {str} {
    global ::id
    set id $str
    set ::issue [exec curl -sSL -u $::creds "https://$::subdomain.atlassian.net/rest/api/latest/issue/$id"]
}

proc description {} {
    set a1 [exec jq -r {[.fields.description] | @tsv} << $::issue]

    puts $a1
}
proc d {} {
    description
}

# puts "Enter ticket id"
# set id [gets stdin]
while {1} {
    puts "$id >"
    set input [gets stdin]
    eval $input
}
