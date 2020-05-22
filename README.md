# DIPI - Vipassana Registration System

DIPI is a Pali word meaning 'Teaching'. This is an online registration system designed for Vipassana Meditation centres around the world.

## Key Features
* SMS Gateway – The system can send an SMS to applicants who do not have an email. The system can also re-confirm, cancel and transfer applications based on student's SMS. 
For e.g – The student can SMS 'Cancel' from their registered mobile number to cancel his/her application. 
* Auto-close/waitlist Courses – The user can define the number of applications they wish to have per course, after which the system can automatically mark the course as FULL/Waitlist
* Course History – The centers can view when a student has applied for a course anywhere in India 
* Supports multiple registration workflows – The centers can choose to process applications in various ways. Both Pre-confirmation/Re-confirmation are supported and students will be notified automatically as defined by the user. Reminder mails can also be sent automatically.
* Cancellation/Re-confirmation via link - Students can cancel/re-confirm their application via a link sent to them in their email correspondence and status will be updated automatically in the system
* AT Scheduling - Centers can create this schedule for all courses to keep track of the conducting ATs
* User Permissions - There are 4 levels of permissions -
  - Registrar - Full Access
  - Data-entry operator - Limited Access
  - Zero Day operator - Can access only zero day section for Day-0 activities such as room allocation, cell allocation, reports etc.
  - View only Access
* Referral List - System automatically highlights the record of a student who has been referral listed
* Transfer Applications - Centers can transfer an application to any other center if requested by the student.

## Getting Started

These instructions will get you a copy of the project up and running on your local machine for development and testing purposes.
<ul>
<li>Install LAMP/WAMP stack</li>
<li>Download <a href="https://github.com/VipassanaTech/db-dump/blob/master/dipi-dump.zip">db dump</a>. The zip file is password protected. Please send an email to <a href="mailto:dipi@vipassana.co">dipi@vipassana.co</a>, if you want access.</li>
<li>To clone this project, you'll need <a href="https://git-scm.com">Git</a> installed on your computer. From your command line:</li>

```bash
# Clone this repository
$ git clone https://github.com/VipassanaTech/dipi-web.git
```

<li>Setup Database</li>

```bash
$ mysql -u root -p
# Enter password
$ create database database_name;
$ quit

# Import database dump
$ mysql -u root -p database_name < path_to_dipi-dump.sql
# Enter password
$ mysql -u root -p
# Enter password
$ grant all privileges on database_name.* to dipi@localhost identified by 'adsklfjajsdkfj';
$ flush privileges;
$ quit

# Copy default.settings.php
# CD into the cloned repo
cd path_to_cloned_repo/sites/default
cp default.settings.php settings.php
# Edit & Update settings.php with the database credentials
```

<li>Update Apache configuration and Host file</li>
</ul>

## Download

You can [download](https://github.com/VipassanaTech/dipi-web/archive/master.zip) the latest version of DIPI for Windows, macOS and Linux.

## Built With

* [Drupal](https://www.drupal.org/drupal-7.0)

## Contributing

Please read [CONTRIBUTING.md](https://github.com/VipassanaTech/dipi-web/blob/master/CONTRIBUTING.md) for details on our code of conduct, and the process for submitting pull requests to us.

## License

This project is licensed under the GPL License

## Copyleft

This project follows the [Copyleft](https://www.gnu.org/copyleft) principle.

Here's a [Comprehensive Guide](https://copyleft.org/guide/comprehensive-gpl-guide.pdf), to understand it in detail.
