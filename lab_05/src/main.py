import argparse
import os
import smtplib
import sys
from email.mime.application import MIMEApplication
from email.mime.multipart import MIMEMultipart
from email.mime.text import MIMEText
from os.path import basename
from time import sleep


def get_args():
    parser = argparse.ArgumentParser()
    parser.add_argument('mail_to', action="store", help="Email address: to")
    parser.add_argument('mail_from', action="store", help="Email address: from")
    parser.add_argument('password_from', action="store", help="Password: from")
    parser.add_argument('-a', '--attachment', action="store", required=False, help="Attach file")
    return parser


def add_file(msg, fname):
    with open(fname, "rb") as fl:
        part = MIMEApplication(fl.read(), Name=basename(fname))
    part['Content-Disposition'] = 'attachment; filename="%s"' % basename(fname)
    msg.attach(part)

    return msg


def send_mail(msg, msg_text, smtphost, mail_from, password_from, mail_to, interval, attachment=None):
    msg['From'] = mail_from
    msg['To'] = mail_to
    msg['Subject'] = "BMSTU CN COURSE LW05"
    msg.attach(MIMEText(msg_text, 'plain'))

    if attachment is not None:
        msg = add_file(msg, attachment)

    server = smtplib.SMTP(smtphost[0], smtphost[1])
    server.starttls()
    server.login(mail_from, password_from)

    while True:
        server.sendmail(msg['From'], msg['To'], msg.as_string())
        print("Email sent from %s to %s" % (msg['From'], msg['To']))
        sleep(interval)

    server.quit()


def main():
    parser = get_args()

    try:
        args = parser.parse_args(sys.argv[1:])
    except:
        sys.exit("Error in sending email: arguments are not valid")

    message = MIMEMultipart()
    message_text = input("Please, enter e-mail message: ")
    interval = int(input("Please, enter interval in seconds: "))
    smtphost = ["smtp.mail.ru", 25]

    send_mail(message, message_text, smtphost, args.mail_from, args.password_from, args.mail_to, interval, args.attachment)


if __name__ == '__main__':
    main()
