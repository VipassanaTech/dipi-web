(function ($) {

	function check_pincode( pin )
	{
		var PinLength=pin.length;
		var url = Drupal.settings.basePath+"patrika/getDetailsByZipcode"; // the script where you handle the form input.
 	
		if(PinLength == 6)
		{
			var PinCode=pin;
	
	       jQuery.ajax({
           type: "POST",
           url: url,
           data: {'PinCode':PinCode}, // serializes the form's elements.
           success: function(data)
           {
               	//console.log(data); // show response from the php script.
           		var obj=JSON.parse(data);
		        if(obj.count > 0)
		        {

		        	jQuery("#student_regi_country").val(obj['data'][0]['c_country']);
		           	if(obj.StateCount > 0)
		           	{
		           		BindDropdown='';
		           		$('#student_regi_state').html('');
			        	jQuery.each(obj.state,function(key,val)
			        	{

			        		if(val.s_code == obj['data'][0]['c_state'])
			        		{
			        			BindDropdown += '<option value="' + val.s_code+ '" selected="selected">' +val.s_name + '</option>';
			        		}
			        		else
			        		{	
	           			 		BindDropdown += '<option value="' + val.s_code+ '">' +val.s_name + '</option>';
			        		}
           				});
        				BindDropdown += '<option value="">Choose</option>';
        				$('#student_regi_state').html(BindDropdown);
		           	}
		           	if(obj.CityCount > 0)
		           	{
		           		BindDropdown='';
		           		$('#student_regi_city').html('');
			        	jQuery.each(obj.city,function(key,val)
			        	{

			        		if(val.c_id == obj['data'][0]['c_id'])
			        		{
									BindDropdown += '<option value="' + val.c_id+ '" selected="selected">' +val.c_name + '</option>';
			        		}
			        		else
			        		{	
	         
	           			 		 BindDropdown += '<option value="' + val.c_id+ '">' +val.c_name + '</option>';
			        		}
           				});
        				BindDropdown += '<option value="">Choose</option><option value="Other">Other</option>';
        				jQuery('#student_regi_city').html(BindDropdown);
        				jQuery('#student_regi_city').val(obj['data'][0]['c_id']).change();


		           	}
		        }
		        else
		        {

		        	$('#student_regi_country').val('').change();
					$('#student_regi_city').html('<option value="">Choose</option>');
					
		        }
           }
         });
		}

	}

  $(document).ready(function(){
    // alert("HELLO");
     if ($("#student_regi_zip").length)
     	check_pincode($("#student_regi_zip").val());

     $(".delete-pid").click(function(){
     	var result = confirm("Are you sure you want to delete payment id "+$(this).attr("pid")+" ?");
     	return result;
     });

     $(".delete-nid").click(function(){
     	var result = confirm("Are you sure you want to delete this note ?");
     	return result;
     });

	 jQuery("#student_regi_country").change(function(){
 		var optionSelected = $(this).find("option:selected");
     	var countrySelected  = optionSelected.val();
     	var url = Drupal.settings.basePath+"patrika/get-state"; 
     	// BindStateDropdown(countrySelected,'','');
	    jQuery.ajax({
	           type: "POST",
	           url: url,
	           data: {'Country':countrySelected},
	           success: function(data)
	           {
	            
	           		var obj=JSON.parse(data);
	           		var BindDropdown = '';
       
            			BindDropdown += '<option value="">Choose</option>';
	           		jQuery.each(obj,function(key,val){
	           			 BindDropdown += '<option value="' + val.s_code+ '">' +val.s_name + '</option>';
	           		});
            		$('#student_regi_state').html(BindDropdown);
	           }
	         });

	  	});

	 	jQuery("#student_regi_state").change(function(){
 		var optionSelected = $(this).find("option:selected");
     	var stateSelected  = optionSelected.val();
     	// BindCityDropdown(stateSelected,'');
     	var url = Drupal.settings.basePath+"patrika/get-city"; 
   
	    jQuery.ajax({
	           type: "POST",
	           url: url,
	           data: {'State':stateSelected}, 
	           success: function(data)
	           {
	             
	           		var obj=JSON.parse(data);
	           		var BindDropdown = '';
       
            			BindDropdown += '<option value="">Choose</option>';
	           		jQuery.each(obj,function(key,val){
	           			 BindDropdown += '<option value="' + val.c_id+ '">' +val.c_name + '</option>';
	           		});
	           			BindDropdown += '<option value="Other">Other</option>';
	           		
            		$('#student_regi_city').html(BindDropdown);
	           }
	         });

	  	});
	  	jQuery("body").delegate("#student_resubscription", "change", function(){
	  			
	  			jQuery('#student_resub_start_date').datepicker('setDate', null);
	  			jQuery('#student_resub_end_date').datepicker('setDate', null);

	  	});
	  	jQuery("#student_subscription").change(function(){
	  			
	  			jQuery('.student_sub_start_date').datepicker('setDate', null);
	  			jQuery('.student_sub_end_date').datepicker('setDate', null);

	  	});
	  	jQuery("body").delegate(".student_resub_start_date", "focusin", function(){
	  		
       		jQuery('.student_resub_end_date').datepicker({ dateFormat: 'dd/mm/yy' });
			jQuery('.student_resub_start_date').datepicker({ dateFormat: 'dd/mm/yy', 
				minDate: 0,
        		onSelect: function (selectedDate ) {
            		var dt2 = jQuery('.student_resub_end_date');
            			var SubVal=jQuery('#student_resubscription').val();
            			var date2 = jQuery('.student_resub_start_date').datepicker('getDate', '+1d'); 
  						if(SubVal == 'Life Long')
  						{
  							date2.setFullYear(date2.getFullYear()+25); 
  							dt2.datepicker( 'option', 'maxDate', date2);
  							jQuery('.student_resub_end_date').datepicker('setDate', date2);
  						
  						}
  						if(SubVal == 'Yearly')
  						{
  							date2.setFullYear(date2.getFullYear()+1); 
  							dt2.datepicker( 'option', 'maxDate', date2);
  							jQuery('.student_resub_end_date').datepicker('setDate', date2);
  						}
  						if(SubVal == 'Privilage')
  						{
  							date2.setFullYear(date2.getFullYear()+25); 
  							dt2.datepicker( 'option', 'maxDate', date2);
  						}
            	       dt2.datepicker( 'option', 'minDate', selectedDate);
            	
            		}
            	});
    	});
    	jQuery("body").delegate(".datepicker", "focusin", function(){
    		jQuery('.datepicker').datepicker({ dateFormat: 'dd/mm/yy' });//receipt date
	  	});
	  	


    	jQuery("#student_regi_zip").keyup(function(){
    		check_pincode(jQuery(this).val());
    	});

    	jQuery("#student_regi_city").change(function(){
    		var optionSelected = $(this).find("option:selected");
     		var citySelected  = optionSelected.val();
     		if(citySelected == 'Other')
     		{
     			jQuery('#student_regi_other_city').show();
     		}
     		else
     		{
     			jQuery('#student_regi_other_city').hide();	
     		}
    	});

  });
})(jQuery);