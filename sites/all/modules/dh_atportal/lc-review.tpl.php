<div class="applicant-details">
  	<table class="table table-hover">
		<tbody>
			<tr class="align-center">
				<td colspan="3"><h2><?php echo $data['Name']?><?php echo $data['a_uri']; ?></h2></td>
				<td rowspan="7"><img src="<?php echo $data['a_photo']; ?>"></td>
			</tr>
			<tr class="align-left">
				<td><label>Course</label></td>
				<td colspan="2"><?php echo $data['c_name']?></td>
			</tr>
			<tr class="align-left">
				<td><label>Email | Contact</label></td>
				<td colspan="2"><?php echo $data['a_email']." | ".$data['a_phone_mobile'];?></td>
			</tr>
			<tr class="align-left">
				<td><label>Location</label></td>
				<td colspan="2"><?php echo $data['City'].", ".$data['State'].", ".$data['Country']?></td>
			</tr>
			<tr class="align-left">
				<td><label>Date of Birth</label></td>
				<td colspan="2"><?php echo $data['DOB']."&nbsp;(&nbsp;".$data['Age']." yrs&nbsp;)&nbsp;";?></td>
			</tr>
			<tr class="align-left">
				<td><label>Recommending AT Status</label></td>
				<td colspan="2"><?php echo $data['al_reco_status'];?></td>
			</tr>
			<tr class="align-left">
				<td><label>Area Teacher Status</label></td>
				<td colspan="2"><?php echo $data['al_area_status'];?></td>
			</tr>
			<tr>
					<table class="course-details table table-hover">
						<tr>
							<td colspan="9"><h3>Course Details</h3></td>
						</tr>
						<tr>
							<td><label>10d</label></td>
							<td><label>STP</label></td>
							<td><label>20d</label></td>
							<td><label>30d</label></td>
							<td><label>45d</label></td>
							<td><label>60d</label></td>
							<td><label>Service</label></td>
							<td><label>10SPL</label></td>
							<td><label>TSC</label></td>
						</tr>
						<tr class="align-right">
							<?php
							   $course_types = array('ac_10d', 'ac_stp', 'ac_20d', 'ac_30d', 'ac_45d', 'ac_60d', 'ac_service', 'ac_spl', 'ac_tsc');
							   foreach($course_types as $c)
								  echo "<td>".$data[$c]."</td>";
							?>
						</tr>
					</table>
			</tr>
		</tbody>
	</table>
	<table>
		<?php
				$fields_no = array('al_committed' => 'Committed to tradition?', 'al_exclusive_2yrs' => 'Practicing Vipassana exclusively for 2 years?', 'al_5_precepts' => 'Maintained 5 precepts?', 'al_intoxicants' => 'Abstained from intoxicants?', 'al_sexual_misconduct' => 'Abstained from sexual misconduct?', 'al_spouse_approve' => 'Spouse Approves?');
				$fields_yes = array( 'al_left_course' => 'Left course before?','al_left_course_details' => 'Left course details', 'al_reduce_practice' => 'Asked to reduce practice?', 'al_personal_tragedy' => 'Recent personal tragedy');
				$i = 1;
					
				foreach( $fields_no as $f => $l )
				if ($data[$f] == '0') 
				{
				  	if ($i == '1') 
				  	{
					  	print "<tr><td colspan='3'><h3>Applicant does not fulfil the following criteria</h3><br>(Please discuss with the applicant & proceed)</td></tr>";
				  		$i--;
				  	}
		?>
					<tr class="align-right">
						<td><h3><?php print $l;?>:</h3></td>
						<td>&nbsp;&nbsp;&nbsp;&nbsp;</td>
						<td><h3>No</h3></td>
					</tr>
		<?php
				}

					foreach( $fields_yes as $f => $l)
						if ($data[$f] == '1')
						{
		?>
						<tr>
							<td><h3><?php print $l; ?>:</h3></td>
							<td>&nbsp;&nbsp;&nbsp;&nbsp;</td>
							<td><h3>Yes</h3></td>
						</tr>
		<?php } 
		?>
	</table>
	<?php
		if ($data['al_recommending_comments']):
	?>
		<h3>Recommending AT Comments:</h3>
		<h4><?php print $data['al_recommending_comments']?></h4>
	 <?php endif ?>

	<?php
		if ($data['al_area_at_comments']):
	?>
		<h3>Area AT Comments:</h3>
		<h4><?php print $data['al_area_at_comments']?></h4>
	 <?php endif ?>

</div>