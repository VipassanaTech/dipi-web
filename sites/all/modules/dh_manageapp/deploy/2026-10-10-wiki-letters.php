<?php
// ============================================================================
// Help for letters: the "Letters" wiki page (Drupal book, bid 10), with everything about
// letters: where they are, when DIPI sends them, which letter is used, writing a letter,
// merge fields (DIPI's, the centre's own, combined fields), the merge-field check and the
// preview, the sender name, WhatsApp, and how to see what was sent.
//
// Run from the Drupal root, on the command line only (php7.3 on prod, which has DOMDocument;
// php7.4 locally):
//     php7.3 sites/all/modules/dh_manageapp/deploy/2026-10-10-wiki-letters.php
// Re-runnable: updates the page if it exists (matched by title in book 10), and sets the
// alias and the root-TOC link once.
// ============================================================================
if (php_sapi_name() !== 'cli') { echo "Command line only.\n"; exit(1); }
define('DRUPAL_ROOT', getcwd());
$_SERVER['REMOTE_ADDR'] = '127.0.0.1'; $_SERVER['HTTP_HOST'] = 'localhost'; $_SERVER['SCRIPT_NAME'] = '/index.php';
require_once DRUPAL_ROOT . '/includes/bootstrap.inc';
drupal_bootstrap(DRUPAL_BOOTSTRAP_FULL);

$title = 'Letters';
$alias = 'wiki-letters';
$body = <<<'HTML'
<p>DIPI sends emails to applicants. These emails are called <b>letters</b>: for example when someone applies, when you confirm them, before the course and after the course. Each centre writes its own letters.</p>

<p><b>Contents</b></p>
<ol>
<li><a href="#l-where">Where to find your letters</a></li>
<li><a href="#l-when">When DIPI sends a letter</a></li>
<li><a href="#l-which">Which letter DIPI uses</a></li>
<li><a href="#l-write">Writing a letter</a></li>
<li><a href="#l-fields">Merge fields</a></li>
<li><a href="#l-own">Your centre's own fields</a></li>
<li><a href="#l-combined">Combined fields</a></li>
<li><a href="#l-check">When a merge field is wrong</a></li>
<li><a href="#l-preview">Preview a letter</a></li>
<li><a href="#l-sender">Sender name, WhatsApp and SMS</a></li>
<li><a href="#l-sent">Checking what was sent</a></li>
</ol>

<h3 id="l-where">1. Where to find your letters</h3>
<p>On your centre page, click <b>Manage Letters</b>. You see two lists:</p>
<ul>
<li><b>Active Letters</b>: the letters DIPI uses.</li>
<li><b>Inactive Letters</b>: letters that are not used and will not be sent. They are not deleted.</li>
</ul>
<p>Next to each letter:</p>
<ul>
<li><b>View</b>: read the letter.</li>
<li><b>Edit</b>: change the letter.</li>
<li><b>Copy</b>: make a new letter from this one.</li>
<li><b>Mark Inactive</b>: stop using the letter. To use it again, click <b>Mark Active</b> in the Inactive Letters list.</li>
</ul>
<p>At the top: <b>Add Letter</b> (a new letter) and <b>Manage Fields</b> (your centre's own merge fields, see <a href="#l-own">section 6</a>).</p>
<p>Only some logins can add or change letters. Some logins can only view them. See <a href="/node/65">Dipi Roles</a>.</p>
<p>A new centre starts with a set of standard letters and fields. Please read them and change them for your centre.</p>

<h3 id="l-when">2. When DIPI sends a letter</h3>
<ul>
<li><b>A new application comes in</b>: the letter for status <b>Received</b>.</li>
<li><b>You change the status</b> on the <a href="/wiki-search">Search</a> page and click <b>Update</b>: the letter for the new status (for example <b>Confirmed</b>, <b>WaitList</b>, <b>Clarification</b>, <b>Cancelled</b>).</li>
<li><b>Before the course</b>, if you turned these on in <a href="/wiki-centre-settings">Centre Settings</a>: the <b>Expected</b> reminder, the <b>ReConfirmation</b> request, and the <b>Cancelled</b> letter when someone does not reply in time.</li>
<li><b>After the course is finalized</b>: the <b>Attended</b> letter to students who attended.</li>
<li><b>Long courses</b>: letters to the Assistant Teachers for review (<b>R-ATReview</b>, <b>A-ATReview</b>).</li>
<li><b>Bulk mail</b>: a <b>Custom</b> letter to many applicants at once. See <a href="/node/72">Sending Bulk-Mails</a>.</li>
</ul>
<p><b>To send a letter without changing the status:</b> on the Search page choose the status <b>Custom</b>, pick the letter, and click <b>Update</b>. The status stays the same.</p>

<h3 id="l-which">3. Which letter DIPI uses</h3>
<p>Every letter has a <b>Status</b> and a <b>Course Type</b>.</p>
<ul>
<li>DIPI uses the letter for that status and that course type (for example Confirmed + 10 Day Course).</li>
<li>If there is none, it uses the letter for that status with Course Type <b>All</b>.</li>
<li>If there is no letter at all for that status, no email is sent.</li>
<li>Inactive letters are never used.</li>
</ul>
<p>On the Search page, when you choose a new status:</p>
<ul>
<li>If only one letter fits, its name shows under the status, so you know which letter will go.</li>
<li>If more than one letter fits, a box <b>Select Letter to send</b> opens. Choose the letter. You can also type an <b>Additional Comment to send</b>. It is put into the letter where the field <b>[UserInput]</b> is.</li>
</ul>

<h3 id="l-write">4. Writing a letter</h3>
<ol>
<li>Click <b>Add Letter</b> (or <b>Edit</b> next to a letter).</li>
<li><b>Letter Name</b>: a name for you, for example "Confirmation 10 Day".</li>
<li><b>Status</b>: when this letter is sent (see section 2). Choose <b>Custom</b> for a letter you send yourself or by bulk mail.</li>
<li><b>Course Type</b>: <b>All</b>, or one course type if this letter is only for that type.</li>
<li><b>Subject</b>: the subject of the email.</li>
<li><b>Letter Body</b>: the email. You can make text bold, add lists, tables and links. Text pasted from Word keeps its look.</li>
<li><b>Pictures</b>: click the picture button and use <b>Upload</b>, or paste a picture into the letter. The picture is put inside the email, so the applicant sees it at once.</li>
<li><b>Attachment</b>: you can attach one file (PDF, Word, Excel or a picture). For <b>Confirmed</b> and <b>Expected</b> letters DIPI also attaches the applicant's application form (PDF) by itself.</li>
<li><b>SMS Text</b>: SMS is not sent any more. You can leave it empty.</li>
<li>Click <b>Add Letter</b> or <b>Update Letter</b> to save.</li>
</ol>

<h3 id="l-fields">5. Merge fields</h3>
<p>A merge field is a word in square brackets that DIPI changes into the applicant's own details when it sends the letter. For example <b>Dear [FirstName]</b> becomes <b>Dear Ravi</b>.</p>
<p><b>How to add a merge field</b></p>
<ol>
<li>In the letter, put your cursor where the field should go (in the Subject, Letter Body or SMS Text).</li>
<li>In the <b>Merge Fields</b> box, click the field. DIPI puts it in for you.</li>
<li>Use <b>Find a field</b> to find a field quickly.</li>
</ol>
<p><b>Rules</b></p>
<ul>
<li>A merge field must be written exactly right: <b>[FirstName]</b> works, <b>[firstname]</b> or <b>[First Name]</b> does not. Clicking the field in the Merge Fields box is the easy way to get it right.</li>
<li>Square brackets <b>[ ]</b> are only for merge fields. For other text use round brackets <b>( )</b>.</li>
</ul>
<p><b>DIPI's merge fields</b></p>
<table>
<tr><th>Field</th><th>What it becomes</th></tr>
<tr><td>[FirstName] [MiddleName] [LastName]</td><td>The applicant's name</td></tr>
<tr><td>[ApplicantType]</td><td>Student or Sevak</td></tr>
<tr><td>[ApplicantGender] [Age]</td><td>Gender and age</td></tr>
<tr><td>[ApplicantEmail] [RegisteredMobile]</td><td>The applicant's email and mobile number</td></tr>
<tr><td>[Applicantadd] [ApplicantCity] [ApplicantState] [ApplicantCountry] [ApplicantPinCode]</td><td>The applicant's address</td></tr>
<tr><td>[ApplicantLang]</td><td>The discourse language</td></tr>
<tr><td>[ApplicantID]</td><td>The application number</td></tr>
<tr><td>[ConfirmationNo]</td><td>The confirmation number</td></tr>
<tr><td>[CourseName] [CourseType] [CourseDescription]</td><td>The course, for example "10 Day Course"</td></tr>
<tr><td>[StartDate] [EndDate]</td><td>Course dates, for example "12th January 2027"</td></tr>
<tr><td>[StartDateNum] [EndDateNum]</td><td>Course dates as numbers, for example "12-01-2027"</td></tr>
<tr><td>[CentreName] [CentreAdd] [CentreCity] [CentreState] [CentreCountry] [CentrePinCode]</td><td>Your centre's name and address</td></tr>
<tr><td>[CentrePhone] [CentreFax] [CentreEmail] [CentreUrl]</td><td>Your centre's phone, fax, email and website</td></tr>
<tr><td>[ViewLink] [AuthLogin] [AuthCode]</td><td>The link and login details the applicant uses to see their application</td></tr>
<tr><td>[LetterDetailURL]</td><td>A link where the applicant can read this letter online</td></tr>
<tr><td>[UserInput]</td><td>The comment you type when you change the status (see section 3)</td></tr>
<tr><td>[ATDetails]</td><td>Names and contact details of the course teachers</td></tr>
<tr><td>[RecommendingAT] [AreaTeacher]</td><td>The teachers named in a long-course application</td></tr>
</table>

<h3 id="l-own">6. Your centre's own fields</h3>
<p>Besides DIPI's fields, every centre has its own merge fields. You see them in the Merge Fields box under <b>Your centre's fields</b>, and you change them on the Manage Letters page with <b>Manage Fields</b>.</p>
<p><b>Fields you already have</b></p>
<p>A new centre starts with a set of fields, together with the standard letters:</p>
<ul>
<li><b>[Footer]</b>: at the bottom of almost all standard letters. It shows your centre's name, address and phone. It is made from DIPI's fields ([CentreName], [CentreAdd], [CentrePhone] and others), so it fills in your centre's details by itself.</li>
<li><b>[ClarificationInformation]</b>, <b>[ConfirmInformation]</b>, <b>[CancelInformation]</b>: a sentence with the link the applicant uses to reply, confirm or cancel.</li>
<li><b>[Student-message]</b> and <b>[Sevak-message]</b>: a message only for students or only for servers (see section 7).</li>
<li>Fields starting with <b>Time-</b>, <b>Day-</b> and <b>Detail-</b>: the course end time, how long before the course the applicant hears from you, and extra notes, one for each course type (see section 7).</li>
</ul>
<p><b>To change a field</b></p>
<ol>
<li>Click <b>Manage Fields</b>.</li>
<li>Click the field, then click <b>Edit</b>.</li>
<li>Change the <b>Field Value</b> and click <b>Update</b>. Every letter that uses this field changes.</li>
</ol>
<p>Do not delete a field that your letters use, for example [Footer]. Those letters would then show the brackets, like "[Footer]", to the applicant.</p>
<p><b>To make a new field</b></p>
<p>For example, bus details that you want in several letters:</p>
<ol>
<li>Click <b>Manage Fields</b>, then <b>New</b>.</li>
<li><b>Field Name</b>: <b>BusDetails</b> (without brackets). <b>Field Value</b>: the text, for example the bus timings from the station.</li>
<li>Click <b>Create</b>. Now you can use <b>[BusDetails]</b> in your letters.</li>
</ol>
<ul>
<li>A field value can use other merge fields, as [Footer] does. DIPI fills those as well.</li>
<li>Make the field first, then use it in a letter. A letter with a field that does not exist yet cannot be saved.</li>
<li>The field name cannot have square brackets, cannot be the same as a DIPI field, and must be different from your other fields.</li>
</ul>

<h3 id="l-combined">7. Combined fields</h3>
<p>Sometimes the text must be different for each course type, for example the time the course ends. For this you put <b>[CourseType]</b> inside another field:</p>
<p><b>... the course ends at [Time-[CourseType]].</b></p>
<p>DIPI first changes <b>[CourseType]</b> into the course type. Then it uses your field with that name:</p>
<table>
<tr><th>Course</th><th>DIPI uses your field</th><th>Example value</th></tr>
<tr><td>10 Day Course</td><td>Time-10 Day Course</td><td>7:00 a.m.</td></tr>
<tr><td>STP Course</td><td>Time-STP Course</td><td>5:00 p.m.</td></tr>
<tr><td>Teenager Course</td><td>Time-Teenager Course</td><td>7:00 a.m.</td></tr>
</table>
<p>So you need <b>one field for every course type you run</b>. If a field is missing, the student sees the brackets, for example "[Time-Children Course]". The letters list warns you about this (see section 8).</p>
<p>The same works with:</p>
<ul>
<li><b>[ApplicantType]</b>: <b>[[ApplicantType]-message]</b> uses your field <b>Student-message</b> or <b>Sevak-message</b>.</li>
<li>Course dates or names: <b>[Bus-[StartDateNum]]</b> uses your field <b>Bus-12-01-2027</b> for the course that starts on 12-01-2027.</li>
</ul>

<h3 id="l-check">8. When a merge field is wrong</h3>
<p>DIPI checks your merge fields, so students do not get letters with "[firstname]" or empty places.</p>
<p><b>When you save a letter</b></p>
<ul>
<li>If a field is wrong, the letter is <b>not saved</b>. A red message shows each wrong field, for example "[firstname] is not a merge field. Did you mean [FirstName]?"</li>
<li>Click <b>Fix ... for me</b> to correct the ones that have a suggestion. Check the letter, then click <b>Update Letter</b> again. Correct the others yourself.</li>
<li>Sometimes a field looks right but has hidden formatting inside (for example half of it is bold). DIPI cleans this by itself when you save, and tells you.</li>
<li>A yellow note after saving means the letter was saved, but something should be added, for example a missing field for a combined field. Add it in <b>Manage Fields</b>.</li>
</ul>
<p><b>On the Manage Letters page</b></p>
<ul>
<li>The <b>Merge Fields</b> column shows each letter's state: <span style="color:#a94442">red "1 to fix"</span> (students will see a wrong field), <span style="color:#8a6d3b">orange "1 warning"</span> (for example a combined field missing for a course type), or <span style="color:#2b7a3d">OK</span>.</li>
<li>Tick <b>Show only these letters</b> to see only the letters that need attention.</li>
<li>Click the red or orange label to open the letter. The <b>Merge field check</b> box lists what to change.</li>
</ul>

<h3 id="l-preview">9. Preview a letter</h3>
<ol>
<li>Open a letter (<b>View</b>).</li>
<li>Click <b>Preview this letter for a course</b>.</li>
<li>Choose one of your courses and <b>Student</b> or <b>Server (Sevak)</b>, then click <b>Show</b>.</li>
</ol>
<p>You see the letter as the applicant will get it, with a sample name ("Sample Student"). Anything shown in <span style="background:#f8d7da;color:#a94442">red</span> is a field DIPI cannot fill for that course. Nothing is sent from the preview.</p>

<h3 id="l-sender">10. Sender name, WhatsApp and SMS</h3>
<ul>
<li><b>Sender</b>: letters come from the <b>Email from name</b> and <b>Email reply to</b> set in <a href="/wiki-centre-settings">Centre Settings</a>. When an applicant replies, the reply goes to that address.</li>
<li><b>WhatsApp</b>: if <b>Enable WhatsApp Messages on Status-Change</b> is on in Centre Settings, DIPI also sends a short WhatsApp message with the applicant's name, the course and the new status (and the confirmation number). This is a fixed message, not your letter, and it cannot be changed. It is not sent for Custom, Expected, Attended and Left.</li>
<li><b>SMS</b>: SMS is not sent any more.</li>
</ul>

<h3 id="l-sent">11. Checking what was sent</h3>
<ul>
<li>Open the application and look at its activity:
<ul>
<li>"Letter sent (Email: ...) - <i>letter name</i>": the letter went out.</li>
<li>"... - unfilled: [<i>field</i>]": the letter went out, but DIPI could not fill that field. Correct the letter or add the field in Manage Fields.</li>
<li>"No letter Found!": there is no letter for that status, so nothing was sent.</li>
<li>"No letter sent, No email or Mobile!": the application has no email address and no mobile number.</li>
</ul></li>
<li>On <a href="/wiki-activity">Daily Activity</a>, choose <b>Letter</b> to see all letters sent.</li>
<li>If an email could not be delivered (for example a wrong email address), the application gets a red <b>Email Error</b> tag. Correct the email address and send the letter again. See <a href="/wiki-tags">Tags</a>.</li>
</ul>
HTML;

// The theme gives tables no borders or spacing, and its fixed top bar hides a heading the
// contents link jumps to: add both inline.
$body = str_replace(
  array('<table>', '<th>', '<td>'),
  array('<table style="border-collapse:collapse;margin:6px 0 12px">',
    '<th style="border:1px solid #ccc;padding:4px 8px;text-align:left;background:#f5f5f5">',
    '<td style="border:1px solid #ccc;padding:4px 8px;vertical-align:top">'),
  $body);
$body = preg_replace('#<h3 id="([^"]+)">#', '<h3 id="$1" style="scroll-margin-top:180px">', $body);

// ---- Wiki page -------------------------------------------------------------
$root_mlid = db_query("select mlid from book where bid = 10 and nid = 10")->fetchField();
if (!$root_mlid) { echo "ERROR: book root (bid=10, nid=10) not found on this site\n"; exit(1); }
$nid = db_query("select b.nid from book b join node n on n.nid = b.nid where b.bid = 10 and n.title = :t", array(':t' => $title))->fetchField();
if ($nid) {
  $node = node_load($nid);
  echo "updating existing page nid=$nid\n";
}
else {
  $node = new stdClass();
  $node->type = 'book';
  node_object_prepare($node);
  $node->uid = 1; $node->status = 1; $node->promote = 0; $node->comment = 0;
  $node->language = LANGUAGE_NONE;
  $w = (int) db_query("select max(weight) from menu_links where plid = :p", array(':p' => $root_mlid))->fetchField();
  $node->book = array('bid' => 10, 'plid' => $root_mlid, 'menu_name' => 'book-toc-10', 'weight' => $w + 1, 'module' => 'book');
  echo "creating new page under root mlid=$root_mlid weight=" . ($w + 1) . "\n";
}
$node->title = $title;
$node->body[LANGUAGE_NONE][0] = array('value' => $body, 'format' => 'full_html');
node_save($node);
echo "saved nid={$node->nid}\n";

$src = 'node/' . $node->nid;
if (!db_query("select 1 from url_alias where source = :s and alias = :a", array(':s' => $src, ':a' => $alias))->fetchField()) {
  $p = array('source' => $src, 'alias' => $alias);
  path_save($p);
  echo "alias set: /$alias -> $src\n";
}
else {
  echo "alias already present\n";
}

// Root TOC (node 10 body): link via the alias, once, before </ol>.
$root = node_load(10);
$rb = $root->body[LANGUAGE_NONE][0]['value'];
if (strpos($rb, 'href="/' . $alias . '"') !== FALSE) {
  echo "root TOC already links the page\n";
}
elseif (strpos($rb, '</ol>') === FALSE) {
  echo "WARN: no </ol> in the root body; add the TOC link by hand\n";
}
else {
  $root->body[LANGUAGE_NONE][0]['value'] = preg_replace('#</ol>#', '<li><a href="/' . $alias . '">' . $title . '</a></li></ol>', $rb, 1);
  $root->body[LANGUAGE_NONE][0]['format'] = 'full_html';
  node_save($root);
  echo "root TOC updated\n";
}

drupal_flush_all_caches();
echo "done. Check /wiki and /$alias\n";
