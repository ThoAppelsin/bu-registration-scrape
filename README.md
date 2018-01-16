This project had been developed to crawl on Boğaziçi University registration system to gather course schedule data,
in C, using the [CURL](https://github.com/curl/curl) library.
The project had been initiated on August of 2014,
during the summer break after my first year at the university.
The project is uploaded to GitHub just now, for the record and publication.

## Purpose

The idea was to gather up all the
[schedule data offered by the registration system of our university](http://registration.boun.edu.tr/schedule.htm),
and generate a database out of them. Asking the registrar office for the data could be an option,
but I believed that the procedures for that would take a while.
Furthermore, such a request could very well also be denied, since;

a) They would not want to bother.
b) There could be information that is not publicly available on the database, so they would have to filter those out
to prevent a possible leak of information, which they again would not want to bother about.

With this data, my aim was to make a much more informative and interactive application for the convenience
of every user of the original system. Among the features planned, following are the ones I recall the most:

1) Preparing a weekly program as a student.
2) Viewing the weekly schedule of a *classroom*.
3) Viewing the weekly schedule of an *instructor*, regarding their lectures.

All these would be trivially possible after the collected data is structured and standardized, and the relations were built.

## Reason for discontinuation

The schedule is displayed in a very cryptic manner on our university's registration pages.
Lack of proper delimiters between days (ranging from Monday to Saturday), slots (ranging from 1 to 12, 13),
and classrooms, were making the deciphering process hard, even for a human reader.

The human reader would initially *guess*, and then later *know* by experience that a string presented as
`AYHAN SAHENK AYHAN SAHENK AYHAN SAHENK` is actually 3 times Ayhan Sahenk, and not 6 separate classes,
alternating between Ayhan and Sahenk.

Similar issues were present with other columns of data presented on the schedule.
**I was able to overcome all those challenges**, and successfully parse those strings through
various techniques.

I discontinued the project for another reason, which became evident to me once I have started to my
first year in my second semester:

1) Lessons with obligatory laboratory sessions were adding extra complexity that I was yet to consider.
2) Depending on the lecture, a student may:
  a) be obliged to attend the laboratory section corresponding to the assigned section of the lecture.
  b) choose from given options on which laboratory section he/she would like to attend.
3) The registration system presents no indicator on the status of the above matter.

By the time I found out about this, my lectures had already begun. I considered this as a critical
issue, and therefore did not want to continue without somehow fixing it. However, the fix required
a design decision to be made, and I did not have the time for it.

## Shortcomings

I had parsed the HTML pages through string searching functions of the standart library.
After years of experience and knowledge, it is very inappropriate and fragile to parse an HTML
document, even with RegEx, let alone through string searching.

Using a combination of HTML tidying library and an XML parser would both be more appropriate
and easier to work with.

## Known Issues

The classrooms on the schedules on the system are now delimited with vertical bars. They are
also now in separate `<span>` elements, so it should be much easier to scrape the schedule web pages now.

My application has been designed to work under slight changes in the system,
such as changes on the ASP endpoints. However, the application was not flexible to
adapt to changes on the schedule table layouts. Attributed to this change on the table layouts,
my application currently fails to properly gather the schedule data.
